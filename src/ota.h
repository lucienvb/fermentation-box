#pragma once

// ArduinoOTA: wireless firmware updates (part of the core, no extra
// libraries). Hostname "fermentatiebox", password from include/secrets.h
// (OTA_PASSWORD). Ota::begin() must only be called after a wifi connection.

namespace Ota {

void begin();
void poll();

// True from onStart until onEnd/onError. Other modules (measurement) must
// skip their work while this is true, so only OTA uses the flash and CPU.
bool isInProgress();

}  // namespace Ota
