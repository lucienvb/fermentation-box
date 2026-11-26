#include <unity.h>

#include <ctime>

#include "camera_gate.h"

using CameraGate::canTakeTestPhoto;
using CameraGate::Reason;

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

void test_allowed_mid_slot_without_history() {
  time_t now = utc(2026, 11, 10, 14, 7, 0);  // slot 14:00, next slot 14:15.
  auto d = canTakeTestPhoto(/*scheduledPhotoBusy=*/false, now, /*intervalMinutes=*/15,
                             /*nowMillis=*/100000, /*hasLastTestPhoto=*/false,
                             /*lastTestPhotoMillis=*/0);
  TEST_ASSERT_TRUE(d.allowed);
}

void test_refused_while_scheduled_photo_busy() {
  time_t now = utc(2026, 11, 10, 14, 7, 0);
  auto d = canTakeTestPhoto(/*scheduledPhotoBusy=*/true, now, 15, 100000, false, 0);
  TEST_ASSERT_FALSE(d.allowed);
  TEST_ASSERT_EQUAL(static_cast<int>(Reason::kSchedulePriority), static_cast<int>(d.reason));
}

void test_refused_within_five_seconds_of_next_slot() {
  time_t now = utc(2026, 11, 10, 14, 14, 56);  // 4 s before the 14:15 slot.
  auto d = canTakeTestPhoto(false, now, 15, 100000, false, 0);
  TEST_ASSERT_FALSE(d.allowed);
  TEST_ASSERT_EQUAL(static_cast<int>(Reason::kSchedulePriority), static_cast<int>(d.reason));
}

void test_allowed_just_outside_five_second_guard() {
  time_t now = utc(2026, 11, 10, 14, 14, 54);  // 6 s before the 14:15 slot.
  auto d = canTakeTestPhoto(false, now, 15, 100000, false, 0);
  TEST_ASSERT_TRUE(d.allowed);
}

void test_allowed_right_after_a_slot_boundary() {
  time_t now = utc(2026, 11, 10, 14, 15, 1);  // just past the slot, next one far away.
  auto d = canTakeTestPhoto(false, now, 15, 100000, false, 0);
  TEST_ASSERT_TRUE(d.allowed);
}

void test_refused_within_rate_limit_window() {
  time_t now = utc(2026, 11, 10, 14, 7, 0);
  auto d = canTakeTestPhoto(false, now, 15, /*nowMillis=*/5000, /*hasLastTestPhoto=*/true,
                             /*lastTestPhotoMillis=*/4000);  // 1 s ago, limit is 3 s.
  TEST_ASSERT_FALSE(d.allowed);
  TEST_ASSERT_EQUAL(static_cast<int>(Reason::kRateLimited), static_cast<int>(d.reason));
  TEST_ASSERT_EQUAL(2, d.retryAfterSeconds);
}

void test_allowed_once_rate_limit_window_passed() {
  time_t now = utc(2026, 11, 10, 14, 7, 0);
  auto d = canTakeTestPhoto(false, now, 15, /*nowMillis=*/8000, /*hasLastTestPhoto=*/true,
                             /*lastTestPhotoMillis=*/4000);  // exactly 4 s ago.
  TEST_ASSERT_TRUE(d.allowed);
}

void test_schedule_priority_over_rate_limit() {
  // Both rules would refuse; schedule priority must win (503, not 429).
  time_t now = utc(2026, 11, 10, 14, 14, 56);
  auto d = canTakeTestPhoto(false, now, 15, 5000, true, 4000);
  TEST_ASSERT_FALSE(d.allowed);
  TEST_ASSERT_EQUAL(static_cast<int>(Reason::kSchedulePriority), static_cast<int>(d.reason));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_allowed_mid_slot_without_history);
  RUN_TEST(test_refused_while_scheduled_photo_busy);
  RUN_TEST(test_refused_within_five_seconds_of_next_slot);
  RUN_TEST(test_allowed_just_outside_five_second_guard);
  RUN_TEST(test_allowed_right_after_a_slot_boundary);
  RUN_TEST(test_refused_within_rate_limit_window);
  RUN_TEST(test_allowed_once_rate_limit_window_passed);
  RUN_TEST(test_schedule_priority_over_rate_limit);
  return UNITY_END();
}
