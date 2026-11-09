#pragma once

// DHT22 on GPIO13. Temporary pin choice for F0 (no SD card yet, see
// NOTES.md); F1 adds the SD card and re-determines which pin stays free in
// 1-bit SD mode. The DHT library itself enforces a minimum read interval of
// 2 seconds (it returns the previous reading otherwise), so this module
// relies on that instead of reimplementing it.

struct SensorReading {
  float temperatureC;
  float humidityPercent;
  bool ok;
};

namespace Sensor {

void begin();
SensorReading read();

}
