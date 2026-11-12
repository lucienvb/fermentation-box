#include "camera_settings.h"

#include <Preferences.h>

namespace {

Preferences prefs;
constexpr const char* kNamespace = "camera";

int clampExposureValue(int value) {
  return value < 0 ? 0 : (value > 1200 ? 1200 : value);
}

int clampGainValue(int value) {
  return value < 0 ? 0 : (value > 30 ? 30 : value);
}

CameraSettings::WhiteBalance clampWhiteBalance(int value) {
  if (value < 0 || value > 4) {
    return CameraSettings::WhiteBalance::kAuto;
  }
  return static_cast<CameraSettings::WhiteBalance>(value);
}

}  // namespace

namespace CameraSettings {

void begin() {
  prefs.begin(kNamespace, /*readOnly=*/false);
}

Settings load() {
  Settings s = kDefault;
  s.whiteBalance =
      clampWhiteBalance(prefs.getInt("wb", static_cast<int>(kDefault.whiteBalance)));
  s.exposureAuto = prefs.getBool("aec", kDefault.exposureAuto);
  s.exposureValue = clampExposureValue(prefs.getInt("aecVal", kDefault.exposureValue));
  s.gainAuto = prefs.getBool("agc", kDefault.gainAuto);
  s.gainValue = clampGainValue(prefs.getInt("agcVal", kDefault.gainValue));
  return s;
}

void save(const Settings& settings) {
  prefs.putInt("wb", static_cast<int>(settings.whiteBalance));
  prefs.putBool("aec", settings.exposureAuto);
  prefs.putInt("aecVal", clampExposureValue(settings.exposureValue));
  prefs.putBool("agc", settings.gainAuto);
  prefs.putInt("agcVal", clampGainValue(settings.gainValue));
}

const char* toString(WhiteBalance whiteBalance) {
  switch (whiteBalance) {
    case WhiteBalance::kSunny:
      return "sunny";
    case WhiteBalance::kCloudy:
      return "cloudy";
    case WhiteBalance::kOffice:
      return "office";
    case WhiteBalance::kHome:
      return "home";
    case WhiteBalance::kAuto:
    default:
      return "auto";
  }
}

}  // namespace CameraSettings
