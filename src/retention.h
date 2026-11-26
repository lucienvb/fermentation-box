#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Pure retention-policy logic: which day directories must be deleted so
// that enough free space remains on the SD card. No Arduino/filesystem
// dependency, so this can be covered by a native test (`pio test -e
// native`). Wired into sd_storage.cpp from F2 onward: deletes the oldest
// /photos/YYYY-MM-DD directories first, never today's directory, never any
// CSV file under /data (callers only ever pass photo day directories in).

namespace Retention {

// Lower bound on SD card free space (see NOTES.md, F2): below this, the
// oldest day photo directories are deleted first. uint64_t: a 32 GB card
// (see NOTES.md) does not fit in uint32_t bytes.
constexpr uint64_t kMinFreeBytes = 500ULL * 1024 * 1024;  // 500 MB

struct DayFileInfo {
  std::string name;  // e.g. "2026-11-04" (a /photos day directory name)
  uint64_t sizeBytes;
};

// Returns the names that must be deleted, oldest first, until
// freeBytes >= minFreeBytes (or nothing is left to delete). `currentDayFile`
// is never selected: that one is still being written to.
std::vector<std::string> filesToDelete(std::vector<DayFileInfo> days, uint64_t freeBytes,
                                        uint64_t minFreeBytes, const std::string& currentDayFile);

}  // namespace Retention
