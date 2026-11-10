#pragma once

#include "esp_camera.h"

// OV2640 camera (AI-Thinker ESP32-CAM). Uses the camera driver already in
// the Arduino ESP32 core (esp_camera.h), no extra library. Pins come
// literally from Espressif's official CameraWebServer example
// (CAMERA_MODEL_AI_THINKER in camera_pins.h). One photo at a time, no live
// stream.

namespace Camera {

// Starts the camera. Logs an error on failure; the rest of the firmware
// (measurement, storage, web page, OTA) keeps working even without a
// (working) camera. capture() then keeps returning a clear error instead of
// crashing.
void begin();

struct PhotoResult {
  bool ok;
  const char* error;  // valid when !ok.
  camera_fb_t* fb;     // valid when ok; caller must call release().
};

// Takes one JPEG photo (SVGA, 800x600). Discards one frame first so exposure
// can adjust. flashBrightnessPercent (0-100, see flash_duty.h): 0 means no
// flash at all; any other value drives the LED (GPIO4, PWM via LEDC) at
// that brightness for a short warmup, discards a frame, takes the photo,
// then turns the LED off immediately - also on every error path below, so
// it never stays on.
PhotoResult capture(int flashBrightnessPercent);

// Returns the framebuffer to the driver. Always call after a successful
// capture(), once the photo has been saved/sent.
void release(camera_fb_t* fb);

// Flash brightness (0-100) used for scheduled photos (main.cpp). Defaults
// to FlashSettings::kDefaultBrightnessPercent until main.cpp loads the
// saved value at boot; the setup page's "Use for schedule" button updates
// it at runtime via setFlashBrightnessPercent.
void setFlashBrightnessPercent(int percent);
int flashBrightnessPercent();

}  // namespace Camera
