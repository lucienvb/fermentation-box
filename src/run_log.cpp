#include "run_log.h"

#include <cstdio>
#include <cstring>

namespace RunLog {

void escapeField(const char* in, char* buf, size_t bufSize) {
  if (bufSize == 0) {
    return;
  }
  if (!in) {
    buf[0] = '\0';
    return;
  }

  if (strpbrk(in, ",\"\n") == nullptr) {
    snprintf(buf, bufSize, "%s", in);
    return;
  }

  size_t pos = 0;
  auto putChar = [&](char c) {
    if (pos + 1 < bufSize) {
      buf[pos++] = c;
    }
  };
  putChar('"');
  for (const char* p = in; *p; ++p) {
    if (*p == '"') {
      putChar('"');
    }
    putChar(*p);
  }
  putChar('"');
  buf[pos] = '\0';
}

namespace {

bool fits(int written, size_t bufSize) {
  return written >= 0 && static_cast<size_t>(written) < bufSize;
}

}  // namespace

bool formatStartLine(const StartFields& f, char* buf, size_t bufSize) {
  char name[40];
  char product[40];
  char note[160];
  escapeField(f.name, name, sizeof(name));
  escapeField(f.product, product, sizeof(product));
  escapeField(f.note, note, sizeof(note));

  int written = snprintf(buf, bufSize, "%ld,start,%s,%s,%.1f,%.1f,%.1f,%.1f,%s",
                          static_cast<long>(f.epoch), name, product, f.starterG, f.flourG,
                          f.waterG, f.waterTempC, note);
  return fits(written, bufSize);
}

bool formatEndLine(time_t epoch, const char* name, const char* note, char* buf, size_t bufSize) {
  char nameBuf[40];
  char noteBuf[160];
  escapeField(name, nameBuf, sizeof(nameBuf));
  escapeField(note, noteBuf, sizeof(noteBuf));

  int written = snprintf(buf, bufSize, "%ld,end,%s,,,,,,%s", static_cast<long>(epoch), nameBuf,
                          noteBuf);
  return fits(written, bufSize);
}

}  // namespace RunLog
