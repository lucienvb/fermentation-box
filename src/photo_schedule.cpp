#include "photo_schedule.h"

namespace PhotoSchedule {

time_t slotStart(time_t utcTime, int intervalMinutes) {
  time_t intervalSeconds = static_cast<time_t>(intervalMinutes) * 60;
  if (intervalSeconds <= 0 || utcTime <= 0) {
    return utcTime;
  }
  // The UTC epoch (1970-01-01 00:00:00) is itself aligned to every minute
  // boundary, so flooring to a multiple of intervalSeconds lands exactly on
  // fixed clock times (e.g. :00/:15/:30/:45) for any interval that divides
  // 60, with no local calendar math needed.
  return (utcTime / intervalSeconds) * intervalSeconds;
}

bool isDue(time_t utcTime, time_t lastCapturedSlotStart, int intervalMinutes) {
  return slotStart(utcTime, intervalMinutes) != lastCapturedSlotStart;
}

}  // namespace PhotoSchedule
