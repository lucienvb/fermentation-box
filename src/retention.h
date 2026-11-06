#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Pure retention-policy logic: which day files must be deleted so that
// enough free space remains. No Arduino/filesystem dependency, so this can
// be covered by a native test (`pio test -e native`). Not used by the
// firmware yet (storage lands in F2); kept here now so the native test
// environment stays meaningful from F0 onward.

namespace Retention {

struct DayFileInfo {
  std::string name;  // e.g. "2026-11-04.csv"
  uint32_t sizeBytes;
};

// Returns the file names that must be deleted, oldest first, until
// freeBytes >= minFreeBytes (or nothing is left to delete). `currentDayFile`
// is never selected: that file is still being written.
std::vector<std::string> filesToDelete(std::vector<DayFileInfo> days, uint32_t freeBytes,
                                        uint32_t minFreeBytes, const std::string& currentDayFile);

}  // namespace Retention
