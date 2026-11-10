#include <Arduino.h>
#include <esp_task_wdt.h>

#include "camera.h"
#include "diag.h"
#include "ota.h"
#include "photo_schedule.h"
#include "sd_storage.h"
#include "sensor.h"
#include "version.h"
#include "web.h"
#include "wifi_time.h"

namespace {

constexpr unsigned long kMeasurementIntervalMs = 60UL * 1000UL;             // 1 minute
constexpr int kPhotoIntervalMinutes = PhotoSchedule::kDefaultIntervalMinutes;  // 15
constexpr unsigned long kSdRetryIntervalMs = 5UL * 60UL * 1000UL;           // 5 minutes
constexpr unsigned long kHourlyMs = 60UL * 60UL * 1000UL;
constexpr uint32_t kWatchdogTimeoutS = 30;
// Gives the DHT22 time to stabilize before the first startup reading.
constexpr unsigned long kSensorWarmupMs = 2000;

unsigned long lastMeasurementMs = 0;
bool hasEverMeasured = false;
time_t lastPhotoSlotStart = 0;  // 0 = nothing captured yet this boot.
bool timeSyncPrinted = false;
bool sdReady = false;
unsigned long lastSdRetryMs = 0;
unsigned long lastRetentionCheckMs = 0;
unsigned long lastStatusMs = 0;
bool bootRecordWritten = false;

uint32_t measurementCount = 0;
uint32_t photoCount = 0;
uint32_t errorCount = 0;

void printStartupDiagnostics() {
  Serial.print("PSRAM found: ");
  Serial.println(psramFound() ? "yes" : "no");
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("Free PSRAM: ");
  Serial.print(ESP.getFreePsram());
  Serial.println(" bytes");
}

void takeMeasurement(time_t nowUtc) {
  SensorReading reading = Sensor::read();
  if (!reading.ok) {
    Serial.println("Measurement failed: DHT22 returned nan (check wiring)");
    ++errorCount;
    return;
  }

  Serial.printf("Measurement  Temperature: %.1f C  Humidity: %.1f %%\n", reading.temperatureC,
                reading.humidityPercent);
  ++measurementCount;

  if (!sdReady) {
    return;
  }
  if (!SdStorage::writeMeasurement(reading, nowUtc)) {
    Serial.println("Storage error: measurement not written (see above)");
    ++errorCount;
    sdReady = false;  // Retried from the main loop, see loop() below.
  }
}

void takePhoto(time_t nowUtc) {
  // Set/cleared around the capture only (not the SD write): CameraGate uses
  // this so a test photo request never lands in the middle of a scheduled
  // one, see camera_gate.h.
  Web::setScheduledPhotoBusy(true);
  unsigned long captureStartMs = millis();
  Camera::PhotoResult photo = Camera::capture(/*withFlash=*/false);
  unsigned long captureMs = millis() - captureStartMs;
  Web::setScheduledPhotoBusy(false);

  if (!photo.ok) {
    Serial.print("Camera error: ");
    Serial.println(photo.error);
    ++errorCount;
    return;
  }

  size_t photoLen = photo.fb->len;
  bool saved = false;
  unsigned long saveMs = 0;
  if (sdReady) {
    unsigned long saveStartMs = millis();
    saved = SdStorage::savePhoto(photo.fb->buf, photo.fb->len, nowUtc);
    saveMs = millis() - saveStartMs;
  }
  Camera::release(photo.fb);

  if (!saved) {
    Serial.println("Storage error: photo not saved (see above)");
    ++errorCount;
    sdReady = false;  // Retried from the main loop, see loop() below.
    return;
  }

  ++photoCount;
  Serial.printf("Photo saved: %u bytes, capture %lu ms, save %lu ms\n",
                static_cast<unsigned>(photoLen), captureMs, saveMs);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.print("Firmware version: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("Reset reason: ");
  Serial.println(Diag::resetReasonString());

  esp_task_wdt_init(kWatchdogTimeoutS, true);
  esp_task_wdt_add(NULL);

  SdStorage::BeginInfo sdInfo = SdStorage::begin();
  sdReady = sdInfo.ok;
  Serial.print("SD card: ");
  Serial.println(sdReady ? "ok" : "failed (see error above, retrying every 5 min)");
  if (sdReady) {
    Serial.printf("SD card size: %llu MB\n",
                  static_cast<unsigned long long>(sdInfo.cardSizeBytes / (1024 * 1024)));
    Serial.printf("SD free space: %llu MB\n",
                  static_cast<unsigned long long>(sdInfo.freeBytes / (1024 * 1024)));
  }

  Camera::begin();  // Prints its own ok/fail, see camera.cpp.

  Sensor::begin();
  delay(kSensorWarmupMs);
  SensorReading initialReading = Sensor::read();
  Serial.print("DHT22: ");
  Serial.println(initialReading.ok ? "ok" : "failed (check wiring)");

  printStartupDiagnostics();

  WifiTime::begin();
  Web::begin();
}

void loop() {
  esp_task_wdt_reset();

  WifiTime::poll();
  if (WifiTime::consumeJustConnected()) {
    Serial.print("Wifi connected, IP address: ");
    Serial.println(WifiTime::ipAddress());
    Ota::begin();
  }

  Ota::poll();
  if (Ota::isInProgress()) {
    // Skip measuring/photo/retention/retry: only OTA should touch the
    // flash and CPU during an update.
    return;
  }

  Web::handleClient();

  unsigned long nowMs = millis();

  if (!sdReady && nowMs - lastSdRetryMs >= kSdRetryIntervalMs) {
    lastSdRetryMs = nowMs;
    Serial.println("SD card: retrying mount");
    SdStorage::BeginInfo sdInfo = SdStorage::begin();
    sdReady = sdInfo.ok;
    Serial.print("SD card: ");
    Serial.println(sdReady ? "ok" : "failed (see error above, retrying again in 5 min)");
  }

  if (!timeSyncPrinted && WifiTime::isTimeSynced()) {
    timeSyncPrinted = true;
    time_t now = WifiTime::nowUtc();
    struct tm tmUtc;
    gmtime_r(&now, &tmUtc);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmUtc);
    Serial.print("Time synced (UTC): ");
    Serial.println(timeBuf);
  }

  if (!WifiTime::isTimeSynced()) {
    return;  // No measurements/photos without synced time (see CLAUDE.md).
  }

  time_t nowUtc = WifiTime::nowUtc();

  if (!bootRecordWritten && sdReady) {
    // Approximate boot epoch: now minus elapsed uptime, so the logged time
    // reflects when the board actually started, not when NTP finished.
    time_t bootEpoch = nowUtc - static_cast<time_t>(nowMs / 1000);
    if (SdStorage::writeBootRecord(bootEpoch, FIRMWARE_VERSION, Diag::resetReasonString())) {
      bootRecordWritten = true;
    } else {
      Serial.println("Storage error: boot record not written (see above)");
      ++errorCount;
      sdReady = false;  // Retried from the main loop above.
    }
  }

  bool measurementDue = !hasEverMeasured || (nowMs - lastMeasurementMs >= kMeasurementIntervalMs);
  if (measurementDue) {
    lastMeasurementMs = nowMs;
    hasEverMeasured = true;
    takeMeasurement(nowUtc);
  }

  if (PhotoSchedule::isDue(nowUtc, lastPhotoSlotStart, kPhotoIntervalMinutes)) {
    lastPhotoSlotStart = PhotoSchedule::slotStart(nowUtc, kPhotoIntervalMinutes);
    takePhoto(nowUtc);
  }

  if (sdReady && nowMs - lastRetentionCheckMs >= kHourlyMs) {
    lastRetentionCheckMs = nowMs;
    SdStorage::enforceRetention(nowUtc);
  }

  if (nowMs - lastStatusMs >= kHourlyMs) {
    lastStatusMs = nowMs;
    uint64_t freeSd = sdReady ? SdStorage::freeSpaceBytes() : 0;
    Serial.printf(
        "Status: %lu photos, %lu measurements, %lu errors since boot; SD free %llu MB; "
        "free heap %u bytes\n",
        static_cast<unsigned long>(photoCount), static_cast<unsigned long>(measurementCount),
        static_cast<unsigned long>(errorCount),
        static_cast<unsigned long long>(freeSd / (1024 * 1024)), ESP.getFreeHeap());
  }
}
