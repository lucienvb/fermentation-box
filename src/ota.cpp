#include "ota.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

#include "secrets.h"

namespace {

bool otaInProgress = false;

}  // namespace

namespace Ota {

void begin() {
  ArduinoOTA.setHostname("fermentatiebox");
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    Serial.println("OTA: update started");
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    Serial.println("OTA: update finished, restarting...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Uploading blocks loop(); resetting the watchdog here prevents it from
    // firing during an update.
    esp_task_wdt_reset();
    Serial.printf("OTA: %u%%\n", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    Serial.print("OTA error: ");
    Serial.println(static_cast<int>(error));
  });

  ArduinoOTA.begin();
  Serial.println("OTA: ready to receive (hostname fermentatiebox)");
}

void poll() {
  ArduinoOTA.handle();
}

bool isInProgress() {
  return otaInProgress;
}

}  // namespace Ota
