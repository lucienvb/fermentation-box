#include "wifi_time.h"

#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"

namespace {

// Timestamps before this threshold (~2023) mean "not synced yet".
constexpr time_t kValidEpochThreshold = 1700000000;

constexpr unsigned long kRetryIntervalMs = 10UL * 1000UL;
constexpr uint8_t kAttemptsBeforeErrorMessage = 3;

bool wasConnected = false;
bool justConnected = false;
unsigned long lastAttemptMs = 0;
uint8_t failedAttempts = 0;
bool errorPrinted = false;

void startConnectAttempt() {
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastAttemptMs = millis();
}

}  // namespace

namespace WifiTime {

void begin() {
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to wifi: ");
  Serial.println(WIFI_SSID);
  startConnectAttempt();
}

void poll() {
  bool connectedNow = (WiFi.status() == WL_CONNECTED);

  if (connectedNow && !wasConnected) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    justConnected = true;
    failedAttempts = 0;
    errorPrinted = false;
  } else if (!connectedNow && wasConnected) {
    Serial.println("Wifi connection lost, reconnecting...");
  }

  if (!connectedNow && millis() - lastAttemptMs >= kRetryIntervalMs) {
    failedAttempts++;
    startConnectAttempt();
    if (failedAttempts >= kAttemptsBeforeErrorMessage && !errorPrinted) {
      Serial.println("Error: cannot connect to wifi. Check SSID/password in include/secrets.h.");
      errorPrinted = true;
    }
  }

  wasConnected = connectedNow;
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool isTimeSynced() {
  return time(nullptr) > kValidEpochThreshold;
}

time_t nowUtc() {
  return time(nullptr);
}

IPAddress ipAddress() {
  return WiFi.localIP();
}

bool consumeJustConnected() {
  bool result = justConnected;
  justConnected = false;
  return result;
}

}  // namespace WifiTime
