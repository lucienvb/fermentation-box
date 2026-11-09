#pragma once

#include <IPAddress.h>
#include <time.h>

// Wifi connection (with automatic reconnect) and time via NTP. Internal time
// is always UTC.

namespace WifiTime {

void begin();
void poll();

bool isConnected();
bool isTimeSynced();
time_t nowUtc();
IPAddress ipAddress();

// Returns true, and resets the flag to false, the first time poll() is
// called after a (re)connect.
bool consumeJustConnected();

}  // namespace WifiTime
