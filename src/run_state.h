#pragma once

#include <cstddef>
#include <ctime>

// Tracks which run (F2e, see NOTES.md) is currently active, in RAM only -
// deliberately not persisted, so a restart during a run loses the
// "currently running" display (NOTES.md documents this as a known
// simplification); /data/runs.csv on the SD card keeps the full history
// regardless. No Arduino dependency, native tested (pio test -e native).

namespace RunState {

struct Current {
  bool active;
  char name[32];
  time_t startEpoch;
};

void start(const char* name, time_t nowUtc);
void end();
Current current();

// Seconds since start(); 0 when no run is active or nowUtc is before the
// recorded start (clock not yet synced).
unsigned long elapsedSeconds(time_t nowUtc);

}  // namespace RunState
