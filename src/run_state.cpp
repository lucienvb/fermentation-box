#include "run_state.h"

#include <cstdio>

namespace RunState {

namespace {
Current gCurrent = {false, "", 0};
}  // namespace

void start(const char* name, time_t nowUtc) {
  gCurrent.active = true;
  snprintf(gCurrent.name, sizeof(gCurrent.name), "%s", name ? name : "");
  gCurrent.startEpoch = nowUtc;
}

void end() {
  gCurrent.active = false;
  gCurrent.name[0] = '\0';
  gCurrent.startEpoch = 0;
}

Current current() {
  return gCurrent;
}

unsigned long elapsedSeconds(time_t nowUtc) {
  if (!gCurrent.active || nowUtc < gCurrent.startEpoch) {
    return 0;
  }
  return static_cast<unsigned long>(nowUtc - gCurrent.startEpoch);
}

}  // namespace RunState
