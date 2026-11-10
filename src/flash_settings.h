#pragma once

// Persists the flash-LED brightness used for the scheduled photos, so the
// setup page's "Use for schedule" button survives a restart (see NOTES.md
// F2c). Backed by Preferences (NVS), part of the Arduino ESP32 core - no
// new library.

namespace FlashSettings {

constexpr int kDefaultBrightnessPercent = 15;

// Opens the "flash" Preferences namespace. Call once from setup().
void begin();

// Clamped to [0, 100] (see flash_duty.h). Returns kDefaultBrightnessPercent
// if nothing was saved yet.
int loadBrightnessPercent();

// Clamps, then persists across restarts.
void saveBrightnessPercent(int percent);

}  // namespace FlashSettings
