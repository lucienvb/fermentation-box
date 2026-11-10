#include "web.h"

#include <Arduino.h>
#include <WebServer.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "camera.h"
#include "camera_gate.h"
#include "paths.h"
#include "photo_schedule.h"
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

  Camera::PhotoResult result = Camera::capture(/*withFlash=*/false);
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

  server.sendHeader("Cache-Control", "no-store");
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

}  // namespace

namespace Web {

void begin() {
  server.on("/", handleRoot);
  server.on("/api/photo", HTTP_GET, handleApiPhoto);
  server.on("/api/photo/reference", HTTP_POST, handleApiPhotoReference);
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
