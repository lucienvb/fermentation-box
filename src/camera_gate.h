#pragma once

#include <ctime>

// Pure function deciding whether a test photo (setup page, F2b) may be taken
// right now. No Arduino/camera dependency, native tested (pio test -e
// native), written before web.cpp uses it.
//
// Two rules, both from CLAUDE.md/NOTES.md: the fixed photo schedule always
// wins (busy, or due within a few seconds), and test photos are rate-limited
// on their own (at most one per kMinTestPhotoIntervalMs).

namespace CameraGate {

constexpr unsigned long kMinTestPhotoIntervalMs = 3000;

// How close to the next scheduled slot a test photo is refused, so it can
// never land in the middle of (or just before) a scheduled capture.
constexpr time_t kScheduleGuardSeconds = 5;

enum class Reason {
  kAllowed,
  kSchedulePriority,  // 503 Service Unavailable
  kRateLimited,       // 429 Too Many Requests
};

struct Decision {
  bool allowed;
  Reason reason;
  unsigned long retryAfterSeconds;  // valid when !allowed.
};

// scheduledPhotoBusy: true while main.cpp is in the middle of a scheduled
// capture. nowMillis/hasLastTestPhoto/lastTestPhotoMillis: millis()-based
// rate limit state for test photos, kept by the caller (web.cpp).
Decision canTakeTestPhoto(bool scheduledPhotoBusy, time_t nowUtc, int photoIntervalMinutes,
                           unsigned long nowMillis, bool hasLastTestPhoto,
                           unsigned long lastTestPhotoMillis);

}  // namespace CameraGate
