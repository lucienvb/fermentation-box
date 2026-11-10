#include "camera.h"

#include <Arduino.h>

#include "flash_duty.h"
#include "flash_settings.h"

namespace {

// Flash LED, AI-Thinker ESP32-CAM: GPIO4 (see NOTES.md). Not part of the
// SD_MMC 1-bit pin set (CLK/CMD/D0 only), so sd_storage.cpp also drives this
// pin low defensively right after mounting the SD card, independent of
// whether Camera::begin() below runs or succeeds (only on the very first
// boot call - see the guard in sd_storage.cpp, since pinMode()/
// digitalWrite() on a pin already attached to LEDC below would detach it).
constexpr int kFlashLedPin = 4;

// Dedicated LEDC channel/timer for the flash, distinct from
// LEDC_CHANNEL_0/LEDC_TIMER_0 used by the camera driver for XCLK below.
constexpr int kFlashLedChannel = 7;
constexpr int kFlashLedFreqHz = 5000;  // Well above flicker/audible range.

// Warmup after turning on the flash, before the discarded frame: gives the
// LED and sensor time to start up/stabilize. Same value as the home-monitor
// project (see NOTES.md).
constexpr unsigned long kFlashWarmupMs = 200;

bool cameraReady = false;
int gFlashBrightnessPercent = FlashSettings::kDefaultBrightnessPercent;

void flashOff() {
  ledcWrite(kFlashLedChannel, 0);
}

void flashOn(int percent) {
  ledcWrite(kFlashLedChannel, FlashDuty::dutyForPercent(percent));
}

// Fetch one frame and discard it immediately, so auto-exposure can adjust
// (to ambient light, or to the flash just turned on) before the real photo
// is taken.
void discardOneFrame() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb) {
    esp_camera_fb_return(fb);
  }
}

}  // namespace

namespace Camera {

void begin() {
  // Explicit off at every boot (also after a watchdog reset): the LED must
  // never come up on by default.
  ledcSetup(kFlashLedChannel, kFlashLedFreqHz, FlashDuty::kResolutionBits);
  ledcAttachPin(kFlashLedPin, kFlashLedChannel);
  flashOff();

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  // Pins for CAMERA_MODEL_AI_THINKER, taken from Espressif's official
  // CameraWebServer example (camera_pins.h in the Arduino ESP32 core,
  // libraries/ESP32/examples/Camera/CameraWebServer/).
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;
  // Fixed choice (see CLAUDE.md/NOTES.md): always SVGA (800x600), no live
  // stream, no higher resolution.
  config.frame_size = FRAMESIZE_SVGA;

  // PSRAM check decides the number of framebuffers and where they live (see
  // NOTES.md for the measurement on this board).
  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    cameraReady = false;
    Serial.print("Camera error: esp_camera_init failed (error code 0x");
    Serial.print(err, 16);
    Serial.println("). Camera stays unavailable, rest keeps working.");
    return;
  }

  cameraReady = true;
  Serial.print("Camera ready (SVGA, PSRAM: ");
  Serial.print(psramFound() ? "yes" : "no");
  Serial.println(").");
}

PhotoResult capture(int flashBrightnessPercent) {
  if (!cameraReady) {
    return {false, "camera not started (see serial monitor at startup)", nullptr};
  }

  bool withFlash = flashBrightnessPercent > 0;
  if (withFlash) {
    flashOn(flashBrightnessPercent);
    delay(kFlashWarmupMs);
  }

  // Always discard one frame first (see discardOneFrame above), even
  // without flash: let exposure adjust before the real photo.
  discardOneFrame();

  camera_fb_t* fb = esp_camera_fb_get();

  // LED off immediately after the photo, on every path below (including
  // errors) - it must never stay on.
  if (withFlash) {
    flashOff();
  }

  if (!fb) {
    return {false, "camera could not take a photo (no frame received)", nullptr};
  }

  if (fb->format != PIXFORMAT_JPEG) {
    esp_camera_fb_return(fb);
    return {false, "camera returned an unexpected image format", nullptr};
  }

  return {true, nullptr, fb};
}

void release(camera_fb_t* fb) {
  if (fb) {
    esp_camera_fb_return(fb);
  }
}

void setFlashBrightnessPercent(int percent) {
  gFlashBrightnessPercent = FlashDuty::clampPercent(percent);
}

int flashBrightnessPercent() {
  return gFlashBrightnessPercent;
}

}  // namespace Camera
