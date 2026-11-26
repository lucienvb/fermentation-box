#pragma once

#include <cstddef>
#include <ctime>

// Pure functions for /data/runs.csv (F2e, see NOTES.md): CSV field
// escaping and line building for labelled runs ("proef-3" etc.), tracked
// alongside the measurements/photos (which never stop or wait for a run).
// No Arduino dependency, native tested (pio test -e native).

namespace RunLog {

// Escapes `in` for a single CSV field (RFC 4180): wraps it in double
// quotes and doubles any inner quotes whenever `in` contains a comma,
// double quote, or newline; otherwise copies it unchanged into buf.
// Truncates rather than overflowing if the result would not fit bufSize.
void escapeField(const char* in, char* buf, size_t bufSize);

struct StartFields {
  time_t epoch;
  const char* name;     // run name, e.g. "proef-3"
  const char* product;  // e.g. "zuurdesem"
  float starterG;
  float flourG;
  float waterG;
  float waterTempC;
  const char* note;  // free text
};

// "epoch,start,name,product,starter_g,flour_g,water_g,water_temp_c,note"
// (no trailing newline - sd_storage.cpp adds one for the file). Free-text
// fields go through escapeField(). Returns false if the formatted line
// would not fit in bufSize.
bool formatStartLine(const StartFields& fields, char* buf, size_t bufSize);

// "epoch,end,name,,,,,,note" - the product/starter/flour/water fields only
// make sense at the start of a run, so they are left empty here.
bool formatEndLine(time_t epoch, const char* name, const char* note, char* buf, size_t bufSize);

}  // namespace RunLog
