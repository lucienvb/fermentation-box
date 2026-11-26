#include "camera_gate.h"

#include "photo_schedule.h"

namespace CameraGate {

Decision canTakeTestPhoto(bool scheduledPhotoBusy, time_t nowUtc, int photoIntervalMinutes,
                           unsigned long nowMillis, bool hasLastTestPhoto,
                           unsigned long lastTestPhotoMillis) {
  if (scheduledPhotoBusy) {
    return {false, Reason::kSchedulePriority, kScheduleGuardSeconds};
  }

  time_t currentSlot = PhotoSchedule::slotStart(nowUtc, photoIntervalMinutes);
  time_t nextSlot = currentSlot + static_cast<time_t>(photoIntervalMinutes) * 60;
  time_t secondsUntilNextSlot = nextSlot - nowUtc;
  if (secondsUntilNextSlot <= kScheduleGuardSeconds) {
    return {false, Reason::kSchedulePriority,
            static_cast<unsigned long>(secondsUntilNextSlot) + 1};
  }

  if (hasLastTestPhoto) {
    unsigned long elapsedMs = nowMillis - lastTestPhotoMillis;  // wraps correctly, unsigned.
    if (elapsedMs < kMinTestPhotoIntervalMs) {
      unsigned long remainingMs = kMinTestPhotoIntervalMs - elapsedMs;
      return {false, Reason::kRateLimited, (remainingMs + 999) / 1000};
    }
  }

  return {true, Reason::kAllowed, 0};
}

}  // namespace CameraGate
