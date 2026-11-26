#include "web.h"

#include <Arduino.h>
#include <WebServer.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "camera.h"
#include "camera_gate.h"
#include "camera_settings.h"
#include "flash_duty.h"
#include "flash_settings.h"
#include "paths.h"
#include "photo_schedule.h"
#include "run_log.h"
#include "run_state.h"
#include "sd_storage.h"
#include "secrets.h"
#include "web_setup_page.h"
#include "wifi_time.h"

namespace {

WebServer server(80);

bool gScheduledPhotoBusy = false;

// Last test photo, copied out of the camera driver's framebuffer right
// after capture so that buffer can be release()d immediately (see
// CLAUDE.md: "release photo buffers immediately"). Lives in PSRAM: a Save
// as reference request then just writes this out, without a second camera
// access. Freed/reallocated on every new test photo.
uint8_t* gLastPhotoBuffer = nullptr;
size_t gLastPhotoLen = 0;
time_t gLastPhotoTakenAtUtc = 0;
bool gHasLastPhoto = false;
unsigned long gLastPhotoMillis = 0;

bool checkAuth() {
  if (server.authenticate(CAMERA_USER, CAMERA_PASSWORD)) {
    return true;
  }
  server.requestAuthentication(BASIC_AUTH, "fermentatiebox-camera");
  return false;
}

void handleRoot() {
  server.send_P(200, "text/html", kSetupPageHtml);
}

void handleApiPhoto() {
  if (!checkAuth()) {
    return;
  }

  time_t nowUtc = WifiTime::nowUtc();
  CameraGate::Decision decision = CameraGate::canTakeTestPhoto(
      gScheduledPhotoBusy, nowUtc, PhotoSchedule::kDefaultIntervalMinutes, millis(),
      gHasLastPhoto, gLastPhotoMillis);

  if (!decision.allowed) {
    bool schedulePriority = decision.reason == CameraGate::Reason::kSchedulePriority;
    server.sendHeader("Retry-After", String(decision.retryAfterSeconds));
    server.send(schedulePriority ? 503 : 429, "text/plain",
                schedulePriority ? "Scheduled photo has priority, try again shortly."
                                  : "Wait a few seconds between test photos.");
    return;
  }

  // Optional ?brightness=0-100 (setup page's flash selector); defaults to
  // whatever the schedule currently uses, so a plain "Take test photo"
  // shows exactly what the next scheduled photo will look like.
  int brightnessPercent = server.hasArg("brightness")
                               ? FlashDuty::clampPercent(server.arg("brightness").toInt())
                               : Camera::flashBrightnessPercent();

  Camera::PhotoResult result = Camera::capture(brightnessPercent);
  if (!result.ok) {
    Serial.print("Camera error: ");
    Serial.println(result.error);
    server.send(503, "text/plain", result.error);
    return;
  }

  size_t photoLen = result.fb->len;
  free(gLastPhotoBuffer);
  gLastPhotoBuffer = static_cast<uint8_t*>(ps_malloc(photoLen));
  bool copied = gLastPhotoBuffer != nullptr;
  if (copied) {
    memcpy(gLastPhotoBuffer, result.fb->buf, photoLen);
  }
  Camera::release(result.fb);

  gLastPhotoMillis = millis();  // A capture happened either way: rate limit applies.
  gHasLastPhoto = copied;
  gLastPhotoLen = copied ? photoLen : 0;
  gLastPhotoTakenAtUtc = nowUtc;

  if (!copied) {
    server.send(500, "text/plain", "Out of memory while buffering the photo.");
    return;
  }

  CameraSettings::Settings settings = Camera::currentSettings();
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("X-Flash-Brightness-Percent", String(brightnessPercent));
  server.sendHeader("X-White-Balance", CameraSettings::toString(settings.whiteBalance));
  server.sendHeader("X-Exposure",
                     settings.exposureAuto ? "auto" : String(settings.exposureValue));
  server.sendHeader("X-Gain", settings.gainAuto ? "auto" : String(settings.gainValue));
  server.setContentLength(gLastPhotoLen);
  server.send(200, "image/jpeg", "");
  server.sendContent(reinterpret_cast<const char*>(gLastPhotoBuffer), gLastPhotoLen);
}

void handleApiPhotoReference() {
  if (!checkAuth()) {
    return;
  }

  if (!gHasLastPhoto || !gLastPhotoBuffer) {
    server.send(409, "text/plain", "No test photo yet - take one first.");
    return;
  }

  if (!SdStorage::saveReferencePhoto(gLastPhotoBuffer, gLastPhotoLen, gLastPhotoTakenAtUtc)) {
    server.send(500, "text/plain", "Could not save reference photo (see serial log).");
    return;
  }

  char path[48];
  Paths::referencePhotoPath(gLastPhotoTakenAtUtc, path, sizeof(path));
  char json[96];
  snprintf(json, sizeof(json), "{\"ok\":true,\"path\":\"%s\"}", path);
  server.send(200, "application/json", json);
}

// Current flash brightness used for scheduled photos (Camera::
// flashBrightnessPercent()), so the setup page can show/preselect it.
void handleApiFlashBrightnessGet() {
  if (!checkAuth()) {
    return;
  }
  char json[48];
  snprintf(json, sizeof(json), "{\"percent\":%d}", Camera::flashBrightnessPercent());
  server.send(200, "application/json", json);
}

// "Use for schedule": persists ?percent=0-100 (NVS, survives a restart) and
// applies it immediately to the next scheduled photo.
void handleApiFlashBrightnessPost() {
  if (!checkAuth()) {
    return;
  }
  if (!server.hasArg("percent")) {
    server.send(400, "text/plain", "Missing percent");
    return;
  }

  int percent = FlashDuty::clampPercent(server.arg("percent").toInt());
  FlashSettings::saveBrightnessPercent(percent);
  Camera::setFlashBrightnessPercent(percent);

  char json[48];
  snprintf(json, sizeof(json), "{\"ok\":true,\"percent\":%d}", percent);
  server.send(200, "application/json", json);
}

// Current white balance/exposure/gain (Camera::currentSettings()), so the
// setup page can show/preselect them.
void handleApiCameraSettingsGet() {
  if (!checkAuth()) {
    return;
  }
  CameraSettings::Settings settings = Camera::currentSettings();
  char json[160];
  snprintf(json, sizeof(json),
           "{\"whiteBalance\":%d,\"exposureAuto\":%s,\"exposureValue\":%d,\"gainAuto\":%s,"
           "\"gainValue\":%d}",
           static_cast<int>(settings.whiteBalance), settings.exposureAuto ? "true" : "false",
           settings.exposureValue, settings.gainAuto ? "true" : "false", settings.gainValue);
  server.send(200, "application/json", json);
}

// "Use for schedule": persists white balance/exposure/gain (NVS, survives a
// restart) and applies them immediately.
void handleApiCameraSettingsPost() {
  if (!checkAuth()) {
    return;
  }

  CameraSettings::Settings settings = CameraSettings::kDefault;
  if (server.hasArg("whiteBalance")) {
    int wb = server.arg("whiteBalance").toInt();
    settings.whiteBalance = (wb >= 0 && wb <= 4) ? static_cast<CameraSettings::WhiteBalance>(wb)
                                                  : CameraSettings::WhiteBalance::kAuto;
  }
  settings.exposureAuto = server.arg("exposureAuto") != "false";
  if (!settings.exposureAuto) {
    settings.exposureValue = constrain(server.arg("exposureValue").toInt(), 0, 1200);
  }
  settings.gainAuto = server.arg("gainAuto") != "false";
  if (!settings.gainAuto) {
    settings.gainValue = constrain(server.arg("gainValue").toInt(), 0, 30);
  }

  CameraSettings::save(settings);
  Camera::applySettings(settings);

  server.send(200, "application/json", "{\"ok\":true}");
}

// Escapes '"' and '\' so a user-supplied run name can be embedded in a JSON
// string literal; truncates rather than overflowing bufSize.
void appendJsonEscaped(const char* in, char* buf, size_t bufSize) {
  size_t pos = 0;
  for (const char* p = in; *p && pos + 2 < bufSize; ++p) {
    if (*p == '"' || *p == '\\') {
      buf[pos++] = '\\';
    }
    buf[pos++] = *p;
  }
  buf[pos] = '\0';
}

// "Start run": logs a run/start line to /data/runs.csv (run_log.h) and
// starts tracking it in RunState (RAM only, see run_state.h) so the setup
// page can show it as the ongoing run. Does not touch the measurement/
// photo schedule, which keeps running regardless (see CLAUDE.md).
void handleApiRunStart() {
  if (!checkAuth()) {
    return;
  }

  String name = server.arg("name");
  if (name.length() == 0) {
    server.send(400, "text/plain", "Missing name");
    return;
  }
  String product = server.arg("product");
  String note = server.arg("note");

  time_t nowUtc = WifiTime::nowUtc();
  RunLog::StartFields fields;
  fields.epoch = nowUtc;
  fields.name = name.c_str();
  fields.product = product.c_str();
  fields.starterG = server.arg("starter_g").toFloat();
  fields.flourG = server.arg("flour_g").toFloat();
  fields.waterG = server.arg("water_g").toFloat();
  fields.waterTempC = server.arg("water_temp_c").toFloat();
  fields.note = note.c_str();

  char line[320];
  if (!RunLog::formatStartLine(fields, line, sizeof(line))) {
    server.send(500, "text/plain", "Run line too long to log.");
    return;
  }
  if (!SdStorage::writeRunLine(line)) {
    server.send(500, "text/plain", "Could not write to runs.csv (see serial log).");
    return;
  }

  RunState::start(name.c_str(), nowUtc);
  server.send(200, "application/json", "{\"ok\":true}");
}

// "End run": logs a run/end line for the currently tracked run and clears
// RunState.
void handleApiRunEnd() {
  if (!checkAuth()) {
    return;
  }

  RunState::Current current = RunState::current();
  if (!current.active) {
    server.send(409, "text/plain", "No run is active.");
    return;
  }

  time_t nowUtc = WifiTime::nowUtc();
  String note = server.arg("note");

  char line[320];
  if (!RunLog::formatEndLine(nowUtc, current.name, note.c_str(), line, sizeof(line))) {
    server.send(500, "text/plain", "Run line too long to log.");
    return;
  }
  if (!SdStorage::writeRunLine(line)) {
    server.send(500, "text/plain", "Could not write to runs.csv (see serial log).");
    return;
  }

  RunState::end();
  server.send(200, "application/json", "{\"ok\":true}");
}

// Current run and how long it has been running (RunState, RAM only), so
// the setup page can show it without re-reading the SD card.
void handleApiRunCurrent() {
  if (!checkAuth()) {
    return;
  }

  RunState::Current current = RunState::current();
  char json[96];
  if (current.active) {
    char nameEscaped[64];
    appendJsonEscaped(current.name, nameEscaped, sizeof(nameEscaped));
    snprintf(json, sizeof(json), "{\"active\":true,\"name\":\"%s\",\"elapsedS\":%lu}",
             nameEscaped, RunState::elapsedSeconds(WifiTime::nowUtc()));
  } else {
    snprintf(json, sizeof(json), "{\"active\":false}");
  }
  server.send(200, "application/json", json);
}

}  // namespace

namespace Web {

void begin() {
  server.on("/", handleRoot);
  server.on("/api/photo", HTTP_GET, handleApiPhoto);
  server.on("/api/photo/reference", HTTP_POST, handleApiPhotoReference);
  server.on("/api/flash/brightness", HTTP_GET, handleApiFlashBrightnessGet);
  server.on("/api/flash/brightness", HTTP_POST, handleApiFlashBrightnessPost);
  server.on("/api/camera/settings", HTTP_GET, handleApiCameraSettingsGet);
  server.on("/api/camera/settings", HTTP_POST, handleApiCameraSettingsPost);
  server.on("/api/run/start", HTTP_POST, handleApiRunStart);
  server.on("/api/run/end", HTTP_POST, handleApiRunEnd);
  server.on("/api/run/current", HTTP_GET, handleApiRunCurrent);
  server.begin();
  Serial.println("Web: setup page ready (http://fermentatiebox.local)");
}

void handleClient() {
  server.handleClient();
}

void setScheduledPhotoBusy(bool busy) {
  gScheduledPhotoBusy = busy;
}

}  // namespace Web
