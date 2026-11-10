#pragma once

#include <cstddef>
#include <ctime>

// Pure functions that turn a UTC time into the fixed SD card paths. No
// Arduino/filesystem dependency, so these are covered by a native test
// (pio test -e native), written before sd_storage.cpp uses them.

namespace Paths {

// "/data/YYYY-MM-DD.csv"
void measurementCsvPath(time_t utcTime, char* buf, size_t bufSize);

// "/photos/YYYY-MM-DD"
void photoDirPath(time_t utcTime, char* buf, size_t bufSize);

// "/photos/YYYY-MM-DD/HHMMSS.jpg"
void photoFilePath(time_t utcTime, char* buf, size_t bufSize);

}  // namespace Paths
