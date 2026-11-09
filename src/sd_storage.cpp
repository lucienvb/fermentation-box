#include "sd_storage.h"

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#include "paths.h"

namespace {

// Flash LED pin (see camera.cpp and NOTES.md): confirmed against the
// official ESP-IDF sdmmc_host docs and the arduino-esp32 SD_MMC source that
// SDMMC slot 1 in 1-bit mode only claims CLK/CMD/D0 (GPIO14/15/2); D1
// (GPIO4) is left untouched by the SD driver.
constexpr int kFlashLedPin = 4;

}  // namespace

namespace SdStorage {

BeginInfo begin() {
  pinMode(kFlashLedPin, OUTPUT);
  digitalWrite(kFlashLedPin, LOW);

  BeginInfo info = {false, 0, 0};

  if (!SD_MMC.begin("/sdcard", /*mode1bit=*/true)) {
    Serial.println("SD error: SD_MMC.begin failed (1-bit mode)");
    return info;
  }

  if (!SD_MMC.exists("/data") && !SD_MMC.mkdir("/data")) {
    Serial.println("SD error: could not create /data directory");
  }
  if (!SD_MMC.exists("/photos") && !SD_MMC.mkdir("/photos")) {
    Serial.println("SD error: could not create /photos directory");
  }

  info.ok = true;
  info.cardSizeBytes = SD_MMC.cardSize();
  info.freeBytes = SD_MMC.totalBytes() - SD_MMC.usedBytes();
  return info;
}

bool writeMeasurement(const SensorReading& reading, time_t measuredAtUtc) {
  if (!reading.ok || measuredAtUtc <= 0) {
    return false;
  }

  char path[32];
  Paths::measurementCsvPath(measuredAtUtc, path, sizeof(path));

  File f = SD_MMC.open(path, FILE_APPEND);
  if (!f) {
    Serial.print("SD error: could not open ");
    Serial.println(path);
    return false;
  }

  char line[48];
  int len = snprintf(line, sizeof(line), "%ld,%.1f,%.1f\n", static_cast<long>(measuredAtUtc),
                      reading.temperatureC, reading.humidityPercent);
  size_t written = f.print(line);
  f.close();

  if (len < 0 || written != static_cast<size_t>(len)) {
    Serial.print("SD error: measurement line not fully written to ");
    Serial.println(path);
    return false;
  }
  return true;
}

bool savePhoto(const uint8_t* data, size_t len, time_t takenAtUtc) {
  if (!data || len == 0 || takenAtUtc <= 0) {
    return false;
  }

  char dirPath[24];
  Paths::photoDirPath(takenAtUtc, dirPath, sizeof(dirPath));
  if (!SD_MMC.exists(dirPath) && !SD_MMC.mkdir(dirPath)) {
    Serial.print("SD error: could not create ");
    Serial.println(dirPath);
    return false;
  }

  char filePath[40];
  Paths::photoFilePath(takenAtUtc, filePath, sizeof(filePath));

  File f = SD_MMC.open(filePath, FILE_WRITE);
  if (!f) {
    Serial.print("SD error: could not open ");
    Serial.println(filePath);
    return false;
  }

  size_t written = f.write(data, len);
  f.close();

  if (written != len) {
    Serial.print("SD error: photo not fully written to ");
    Serial.println(filePath);
    return false;
  }
  return true;
}

}  // namespace SdStorage
