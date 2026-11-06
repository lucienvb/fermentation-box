#include <Arduino.h>
#include <esp_task_wdt.h>

#include "diag.h"
#include "ota.h"
#include "sensor.h"
#include "version.h"
#include "wifi_time.h"

namespace {

constexpr unsigned long kMeasureIntervalMs = 60UL * 1000UL;
constexpr uint32_t kWatchdogTimeoutS = 30;

unsigned long lastMeasureMs = 0;
bool hasEverMeasured = false;
bool timeSyncPrinted = false;

void measureAndPrint() {
  SensorReading reading = Sensor::read();

  if (!reading.ok) {
    Serial.println("Measurement failed: DHT22 returned nan (check wiring)");
    return;
  }

  time_t measuredAtUtc = WifiTime::nowUtc();
  struct tm tmUtc;
  gmtime_r(&measuredAtUtc, &tmUtc);
  char timeBuf[32];
  strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmUtc);

  Serial.printf("%s UTC  Temperature: %.1f C  Humidity: %.1f %%\n", timeBuf, reading.temperatureC,
                reading.humidityPercent);
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

  Sensor::begin();
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
    // Skip measuring: only OTA should touch the flash and CPU during an
    // update.
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
  bool intervalElapsed = now - lastMeasureMs >= kMeasureIntervalMs;
  if (WifiTime::isTimeSynced() && (!hasEverMeasured || intervalElapsed)) {
    lastMeasureMs = now;
    hasEverMeasured = true;
    measureAndPrint();
  }
}
