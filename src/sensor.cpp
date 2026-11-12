#include "sensor.h"

#include <Arduino.h>
#include <DHT.h>

namespace {

constexpr uint8_t kDataPin = 13;
DHT dht(kDataPin, DHT22);

// One retry after this delay on a failed read (F2d, see NOTES.md): matches
// the DHT22's own minimum read interval (2 s), so the retry is a real new
// reading rather than the library's cached previous value.
constexpr unsigned long kRetryDelayMs = 2500;

SensorReading readOnce() {
  SensorReading reading;
  reading.temperatureC = dht.readTemperature();
  reading.humidityPercent = dht.readHumidity();
  reading.ok = !isnan(reading.temperatureC) && !isnan(reading.humidityPercent);
  return reading;
}

}  // namespace

namespace Sensor {

void begin() {
  dht.begin();
}

SensorReading read() {
  SensorReading reading = readOnce();
  if (!reading.ok) {
    delay(kRetryDelayMs);
    reading = readOnce();
  }
  return reading;
}

}  // namespace Sensor
