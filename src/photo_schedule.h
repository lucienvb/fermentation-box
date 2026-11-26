#pragma once

#include <ctime>

// Pure functions deciding when a photo is due, on fixed UTC clock times
// (e.g. :00/:15/:30/:45 for a 15-minute interval), never "N minutes after
// boot". No Arduino dependency, native tested (pio test -e native).

namespace PhotoSchedule {

// Default photo interval in minutes (see NOTES.md). Must divide 60 so that
// slot boundaries land on fixed clock times.
constexpr int kDefaultIntervalMinutes = 15;

// Start of the fixed clock slot (aligned to the UTC epoch, so :00/:15/:30/
// :45 for a 15-minute interval) that utcTime falls into.
time_t slotStart(time_t utcTime, int intervalMinutes);

// True if utcTime's slot differs from lastCapturedSlotStart, i.e. no photo
// has been taken yet for this slot. lastCapturedSlotStart == 0 means
// "nothing captured yet this boot" (always due). Callers store the slot's
// own start back into lastCapturedSlotStart after capturing, which both
// prevents a second photo in the same slot and, after a restart within
// that slot, still allows exactly one.
bool isDue(time_t utcTime, time_t lastCapturedSlotStart, int intervalMinutes);

}  // namespace PhotoSchedule
