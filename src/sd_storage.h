#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>

#include "sensor.h"

// SD card storage in SDMMC 1-bit mode: one CSV file per UTC day under /data
// and one JPEG per photo under /photos/YYYY-MM-DD (paths from paths.h). SD
// errors are logged and must never stop the measurement/photo loop (not
// wired into main.cpp's abort path); callers must check the returned bool.

namespace SdStorage {

struct BeginInfo {
  bool ok;
  uint64_t cardSizeBytes;
  uint64_t freeBytes;
};

// Mounts the SD card in 1-bit mode (CLK=GPIO14, CMD=GPIO15, D0=GPIO2 - see
// NOTES.md). GPIO4 (flash LED) is not part of this pin set but is driven
// low here anyway, defensively, independent of whether Camera::begin()
// later succeeds.
BeginInfo begin();

// Appends one line "epoch,temperature,humidity\n" to
// /data/YYYY-MM-DD.csv (day taken from measuredAtUtc). Returns false and
// logs on any error.
bool writeMeasurement(const SensorReading& reading, time_t measuredAtUtc);

// Writes `len` bytes to /photos/YYYY-MM-DD/HHMMSS.jpg (day/time taken from
// takenAtUtc), creating the day directory if needed. Returns false and
// logs on any error.
bool savePhoto(const uint8_t* data, size_t len, time_t takenAtUtc);

}  // namespace SdStorage
