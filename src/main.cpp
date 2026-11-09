#include <Arduino.h>
#include <esp_task_wdt.h>

#include "camera.h"
#include "diag.h"
#include "ota.h"
#include "sd_storage.h"
#include "sensor.h"
#include "version.h"
#include "wifi_time.h"

namespace {

// F1 spike: one combined interval for both the measurement and the photo.
// F2 splits this into a 1-minute measurement schedule and a 15-minute
// photo schedule (both as pure, tested functions).
constexpr unsigned long kCaptureIntervalMs = 60UL * 1000UL;
constexpr uint32_t kWatchdogTimeoutS = 30;
// Gives the DHT22 time to stabilize before the first startup reading.
constexpr unsigned long kSensorWarmupMs = 2000;

unsigned long lastCaptureMs = 0;
bool hasEverCaptured = false;
bool timeSyncPrinted = false;
bool sdReady = false;

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

void captureAndStore() {
  time_t nowUtc = WifiTime::nowUtc();

  SensorReading reading = Sensor::read();
  if (reading.ok) {
    Serial.printf("Measurement  Temperature: %.1f C  Humidity: %.1f %%\n", reading.temperatureC,
                  reading.humidityPercent);
    if (sdReady && !SdStorage::writeMeasurement(reading, nowUtc)) {
      Serial.println("Storage error: measurement not written (see above)");
    }
  } else {
    Serial.println("Measurement failed: DHT22 returned nan (check wiring)");
  }

  unsigned long captureStartMs = millis();
  Camera::PhotoResult photo = Camera::capture(/*withFlash=*/false);
  unsigned long captureMs = millis() - captureStartMs;

  if (!photo.ok) {
    Serial.print("Camera error: ");
    Serial.println(photo.error);
    return;
  }

  size_t photoLen = photo.fb->len;
  unsigned long saveStartMs = millis();
  bool saved = sdReady && SdStorage::savePhoto(photo.fb->buf, photo.fb->len, nowUtc);
  unsigned long saveMs = millis() - saveStartMs;
  Camera::release(photo.fb);

  if (!saved) {
    Serial.println("Storage error: photo not saved (see above)");
    return;
  }

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
  Serial.println(sdReady ? "ok" : "failed (see error above)");
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
    // Skip measuring/photo: only OTA should touch the flash and CPU during
    // an update.
    return;
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

  unsigned long now = millis();
  bool intervalElapsed = now - lastCaptureMs >= kCaptureIntervalMs;
  if (WifiTime::isTimeSynced() && (!hasEverCaptured || intervalElapsed)) {
    lastCaptureMs = now;
    hasEverCaptured = true;
    captureAndStore();
  }
}
