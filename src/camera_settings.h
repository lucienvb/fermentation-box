#pragma once

#include <cstdint>

// Fixed white balance / exposure / gain for the camera (F2d, see NOTES.md),
// chosen on the setup page next to the flash brightness and persisted in
// NVS (Preferences) the same way as flash_settings.h, so "Use for
// schedule" survives a restart. Default stays "auto" for everything
// (today's behavior) until something is explicitly saved.
//
// Values/ranges come straight from Espressif's esp32-camera driver
// (sensor.h, vendored in the Arduino ESP32 core, and sensors/ov2640.c):
// wb_mode 0 (auto) - 4 (Home preset), aec_value 0-1200, agc_gain 0-30. The
// three settings are independent sensor_t calls with no interlocking in
// the driver itself (confirmed in ov2640.c: no setter checks another
// setting's state).

namespace CameraSettings {

enum class WhiteBalance : uint8_t {
  kAuto = 0,
  kSunny = 1,
  kCloudy = 2,
  kOffice = 3,
  kHome = 4,
};

struct Settings {
  WhiteBalance whiteBalance;
  bool exposureAuto;
  int exposureValue;  // 0-1200 (sensor_t::set_aec_value), used when !exposureAuto.
  bool gainAuto;
  int gainValue;  // 0-30 (sensor_t::set_agc_gain), used when !gainAuto.
};

constexpr Settings kDefault = {WhiteBalance::kAuto, true, 0, true, 0};

// Opens the "camera" Preferences namespace. Call once from setup().
void begin();

// Returns kDefault if nothing was saved yet.
Settings load();

// Persists across restarts.
void save(const Settings& settings);

// Human-readable name for the setup page / HTTP response headers (e.g.
// "sunny"); "auto" for WhiteBalance::kAuto.
const char* toString(WhiteBalance whiteBalance);

}  // namespace CameraSettings
