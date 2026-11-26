#pragma once

// Local setup page (F2b): aim and focus the camera before the overnight
// test, without pulling the SD card. Synchronous WebServer (same model as
// the home-monitor project): handleClient() processes one request at a
// time, briefly blocking loop() for the duration of a capture - same as a
// scheduled photo already does. Reachable at http://fermentatiebox.local
// (mDNS hostname already registered by Ota::begin()).

namespace Web {

void begin();
void handleClient();

// Set true right before, and false right after, a scheduled capture in
// main.cpp's takePhoto(). CameraGate uses this so the schedule always wins
// over a test photo request.
void setScheduledPhotoBusy(bool busy);

}  // namespace Web
