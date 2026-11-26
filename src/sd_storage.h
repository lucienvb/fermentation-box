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
// /data/YYYY-MM-DD.csv (day taken from measuredAtUtc), or "epoch,,\n" if
// reading.ok is false (F2d, see NOTES.md) - so a gap in the data is
// explainable instead of silently missing. Returns false and logs on any
// error.
bool writeMeasurement(const SensorReading& reading, time_t measuredAtUtc);

// Writes `len` bytes to /photos/YYYY-MM-DD/HHMMSS.jpg (day/time taken from
// takenAtUtc), creating the day directory if needed. Returns false and
// logs on any error.
bool savePhoto(const uint8_t* data, size_t len, time_t takenAtUtc);

// Writes `len` bytes to /photos/reference/YYYY-MM-DD_HHMMSS.jpg (F2b setup
// page, "Save as reference"), creating the directory if needed. Returns
// false and logs on any error.
bool saveReferencePhoto(const uint8_t* data, size_t len, time_t takenAtUtc);

// Appends one line "epoch,firmware_version,reset_reason\n" to
// /data/boots.csv. Call once per boot, after time has synced (a boot
// record needs a real epoch). Returns false and logs on any error.
bool writeBootRecord(time_t bootEpochUtc, const char* firmwareVersion, const char* resetReason);

// Appends one pre-formatted status line (status_line.h, built once per
// hour by main.cpp) plus a newline to /data/status.csv. Returns false and
// logs on any error.
bool writeStatusLine(const char* line);

// Current free space on the mounted card. Only valid when the card is
// mounted (begin() returned ok); callers must check that themselves.
uint64_t freeSpaceBytes();

// If free space is below Retention::kMinFreeBytes, deletes the oldest
// /photos/YYYY-MM-DD directories (via retention.h's pure policy) until
// enough space is free again. Never deletes nowUtc's own day directory,
// never touches /data (CSV files). Call at most once per hour (see
// main.cpp); this walks the whole /photos directory tree.
void enforceRetention(time_t nowUtc);

}  // namespace SdStorage
