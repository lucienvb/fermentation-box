#include "paths.h"

#include <cstdio>

namespace {

void formatUtc(time_t utcTime, const char* format, char* buf, size_t bufSize) {
  struct tm tmUtc;
  gmtime_r(&utcTime, &tmUtc);
  strftime(buf, bufSize, format, &tmUtc);
}

}  // namespace

namespace Paths {

void measurementCsvPath(time_t utcTime, char* buf, size_t bufSize) {
  char day[11];
  formatUtc(utcTime, "%Y-%m-%d", day, sizeof(day));
  snprintf(buf, bufSize, "/data/%s.csv", day);
}

void photoDirPath(time_t utcTime, char* buf, size_t bufSize) {
  char day[11];
  formatUtc(utcTime, "%Y-%m-%d", day, sizeof(day));
  snprintf(buf, bufSize, "/photos/%s", day);
}

void photoFilePath(time_t utcTime, char* buf, size_t bufSize) {
  char day[11];
  char hms[7];
  formatUtc(utcTime, "%Y-%m-%d", day, sizeof(day));
  formatUtc(utcTime, "%H%M%S", hms, sizeof(hms));
  snprintf(buf, bufSize, "/photos/%s/%s.jpg", day, hms);
}

}  // namespace Paths
