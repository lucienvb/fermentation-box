#include "sensor.h"

#include <DHT.h>

namespace {

constexpr uint8_t kDataPin = 13;
DHT dht(kDataPin, DHT22);

}  // namespace

namespace Sensor {

void begin() {
  dht.begin();
}

SensorReading read() {
  SensorReading reading;
  reading.temperatureC = dht.readTemperature();
  reading.humidityPercent = dht.readHumidity();
  reading.ok = !isnan(reading.temperatureC) && !isnan(reading.humidityPercent);
  return reading;
}

}  // namespace Sensor
