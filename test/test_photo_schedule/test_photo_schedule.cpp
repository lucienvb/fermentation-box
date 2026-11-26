#include <unity.h>

#include <ctime>

#include "photo_schedule.h"

using PhotoSchedule::isDue;
using PhotoSchedule::slotStart;

namespace {

time_t utc(int year, int month, int day, int hour, int minute, int second) {
  struct tm tmUtc = {};
  tmUtc.tm_year = year - 1900;
  tmUtc.tm_mon = month - 1;
  tmUtc.tm_mday = day;
  tmUtc.tm_hour = hour;
  tmUtc.tm_min = minute;
  tmUtc.tm_sec = second;
  return timegm(&tmUtc);
}

}  // namespace

void test_slot_start_on_exact_boundary() {
  time_t t = utc(2026, 11, 9, 14, 15, 0);
  TEST_ASSERT_EQUAL(t, slotStart(t, 15));
}

void test_slot_start_mid_slot_floors_to_boundary() {
  time_t mid = utc(2026, 11, 9, 14, 7, 33);
  time_t boundary = utc(2026, 11, 9, 14, 0, 0);
  TEST_ASSERT_EQUAL(boundary, slotStart(mid, 15));
}

void test_due_exactly_on_slot_boundary() {
  time_t previousSlot = utc(2026, 11, 9, 14, 0, 0);
  time_t now = utc(2026, 11, 9, 14, 15, 0);
  TEST_ASSERT_TRUE(isDue(now, previousSlot, 15));
}

void test_not_due_just_before_boundary() {
  time_t currentSlot = utc(2026, 11, 9, 14, 0, 0);
  time_t now = utc(2026, 11, 9, 14, 14, 59);
  TEST_ASSERT_FALSE(isDue(now, currentSlot, 15));
}

void test_due_just_after_boundary() {
  time_t previousSlot = utc(2026, 11, 9, 14, 0, 0);
  time_t now = utc(2026, 11, 9, 14, 15, 1);
  TEST_ASSERT_TRUE(isDue(now, previousSlot, 15));
}

void test_restart_mid_slot_captures_exactly_once() {
  time_t now = utc(2026, 11, 9, 14, 7, 33);
  // Sentinel for "nothing captured yet this boot": due immediately, even
  // mid-slot, so a restart does not miss the slot entirely.
  TEST_ASSERT_TRUE(isDue(now, /*lastCapturedSlotStart=*/0, 15));

  time_t capturedSlot = slotStart(now, 15);
  time_t stillSameSlot = utc(2026, 11, 9, 14, 9, 0);
  // No second photo once the slot has been captured, restart or not.
  TEST_ASSERT_FALSE(isDue(stillSameSlot, capturedSlot, 15));
}

void test_due_across_utc_day_rollover() {
  time_t lastSlotOfDay = utc(2026, 11, 9, 23, 45, 0);
  time_t firstSlotNextDay = utc(2026, 11, 10, 0, 0, 0);
  TEST_ASSERT_EQUAL(firstSlotNextDay, slotStart(firstSlotNextDay, 15));
  TEST_ASSERT_TRUE(isDue(firstSlotNextDay, lastSlotOfDay, 15));
}

void test_five_minute_interval() {
  time_t mid = utc(2026, 11, 9, 10, 7, 0);
  time_t boundary = utc(2026, 11, 9, 10, 5, 0);
  TEST_ASSERT_EQUAL(boundary, slotStart(mid, 5));
  TEST_ASSERT_TRUE(isDue(mid, utc(2026, 11, 9, 10, 0, 0), 5));
  TEST_ASSERT_FALSE(isDue(mid, boundary, 5));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_slot_start_on_exact_boundary);
  RUN_TEST(test_slot_start_mid_slot_floors_to_boundary);
  RUN_TEST(test_due_exactly_on_slot_boundary);
  RUN_TEST(test_not_due_just_before_boundary);
  RUN_TEST(test_due_just_after_boundary);
  RUN_TEST(test_restart_mid_slot_captures_exactly_once);
  RUN_TEST(test_due_across_utc_day_rollover);
  RUN_TEST(test_five_minute_interval);
  return UNITY_END();
}
