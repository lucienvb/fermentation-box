#include "flash_settings.h"

#include <Preferences.h>

#include "flash_duty.h"

namespace {

Preferences prefs;
constexpr const char* kNamespace = "flash";
constexpr const char* kBrightnessKey = "brightness";

}  // namespace

namespace FlashSettings {

void begin() {
  prefs.begin(kNamespace, /*readOnly=*/false);
}

int loadBrightnessPercent() {
  return FlashDuty::clampPercent(prefs.getInt(kBrightnessKey, kDefaultBrightnessPercent));
}

void saveBrightnessPercent(int percent) {
  prefs.putInt(kBrightnessKey, FlashDuty::clampPercent(percent));
}

}  // namespace FlashSettings
