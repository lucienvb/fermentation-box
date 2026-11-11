#include "flash_duty.h"

namespace FlashDuty {

int clampPercent(int percent) {
  if (percent < 0) {
    return 0;
  }
  if (percent > 100) {
    return 100;
  }
  return percent;
}

int dutyForPercent(int percent) {
  int p = clampPercent(percent);
  // Integer rounding to nearest duty value: (p * kMaxDuty + 50) / 100.
  return (p * kMaxDuty + 50) / 100;
}

}  // namespace FlashDuty
