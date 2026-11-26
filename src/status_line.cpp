#include "status_line.h"

#include <cstdio>

namespace StatusLine {

void formatLine(const Snapshot& s, char* buf, size_t bufSize) {
  snprintf(buf, bufSize, "%ld,%s,%lu,%lu,%lu,%lu,%lu,%llu,%lu,%lu,%s", s.epoch,
           s.firmwareVersion ? s.firmwareVersion : "", static_cast<unsigned long>(s.uptimeS),
           static_cast<unsigned long>(s.counters.readingsOk),
           static_cast<unsigned long>(s.counters.readingsFailed),
           static_cast<unsigned long>(s.counters.photosOk),
           static_cast<unsigned long>(s.counters.photosFailed),
           static_cast<unsigned long long>(s.sdFreeMb), static_cast<unsigned long>(s.freeHeap),
           static_cast<unsigned long>(s.freePsram), s.lastError ? s.lastError : "");
}

}  // namespace StatusLine
