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
// can adjust. When withFlash: LED (GPIO4) on, short warmup, discard a frame,
// take the photo, LED off immediately (also on every error path below) -
// the LED never stays on.
PhotoResult capture(bool withFlash);

// Returns the framebuffer to the driver. Always call after a successful
// capture(), once the photo has been saved/sent.
void release(camera_fb_t* fb);

}  // namespace Camera
