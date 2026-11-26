#pragma once

// Pure flash-brightness math: percent (0-100) to LEDC duty value. No
// Arduino dependency, native tested (pio test -e native). Used by camera.cpp
// (LEDC-driven flash LED on GPIO4, see NOTES.md F2c) and web.cpp (clamping
// percent values coming from HTTP requests).

namespace FlashDuty {

// LEDC resolution for the flash LED channel (see camera.cpp).
constexpr int kResolutionBits = 8;
constexpr int kMaxDuty = (1 << kResolutionBits) - 1;  // 255

// Clamps to [0, 100]. 0 means "no flash at all".
int clampPercent(int percent);

// clampPercent(percent), then scaled to [0, kMaxDuty], rounded to the
// nearest duty value.
int dutyForPercent(int percent);

}  // namespace FlashDuty
