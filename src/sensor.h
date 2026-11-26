#pragma once

// DHT22 on GPIO13. Confirmed free in F1: GPIO13 is the SDMMC D3 line, only
// used in 4-bit SD mode (see NOTES.md); this firmware uses the SD card in
// 1-bit mode. The DHT library itself enforces a minimum read interval of
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
