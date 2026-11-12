#pragma once

#include <cstddef>
#include <cstdint>

// Pure formatting of the hourly status line (F2d, see NOTES.md): written to
// /data/status.csv and the serial monitor. No Arduino dependency, native
// tested (pio test -e native).

namespace StatusLine {

// Since the previous status line (reset to 0 by main.cpp after each line).
struct Counters {
  uint32_t readingsOk;
  uint32_t readingsFailed;
  uint32_t photosOk;
  uint32_t photosFailed;
};

struct Snapshot {
  long epoch;
  const char* firmwareVersion;
  unsigned long uptimeS;
  Counters counters;
  uint64_t sdFreeMb;
  uint32_t freeHeap;
  uint32_t freePsram;
  const char* lastError;  // "" if nothing failed since the previous line.
};

// "epoch,firmware_version,uptime_s,readings_ok,readings_failed,photos_ok,
// photos_failed,sd_free_mb,free_heap,free_psram,last_error" - no trailing
// newline (sd_storage.cpp adds one for the file, Serial::println for the
// monitor). Assumes lastError never contains a comma, true for every error
// string in this firmware (see NOTES.md).
void formatLine(const Snapshot& snapshot, char* buf, size_t bufSize);

}  // namespace StatusLine
