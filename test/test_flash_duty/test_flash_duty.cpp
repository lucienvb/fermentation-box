#include <unity.h>

#include "flash_duty.h"

using FlashDuty::clampPercent;
using FlashDuty::dutyForPercent;
using FlashDuty::kMaxDuty;

void test_clamp_within_range_is_unchanged() {
  TEST_ASSERT_EQUAL(0, clampPercent(0));
  TEST_ASSERT_EQUAL(15, clampPercent(15));
  TEST_ASSERT_EQUAL(100, clampPercent(100));
}

void test_clamp_below_zero() {
  TEST_ASSERT_EQUAL(0, clampPercent(-10));
}

void test_clamp_above_hundred() {
  TEST_ASSERT_EQUAL(100, clampPercent(150));
}

void test_duty_zero_percent_is_off() {
  TEST_ASSERT_EQUAL(0, dutyForPercent(0));
}

void test_duty_hundred_percent_is_max_duty() {
  TEST_ASSERT_EQUAL(kMaxDuty, dutyForPercent(100));
  TEST_ASSERT_EQUAL(255, kMaxDuty);
}

void test_duty_default_fifteen_percent() {
  TEST_ASSERT_EQUAL(38, dutyForPercent(15));  // round(15/100*255) = round(38.25) = 38
}

void test_duty_fifty_percent_rounds_up() {
  TEST_ASSERT_EQUAL(128, dutyForPercent(50));  // round(127.5) = 128
}

void test_duty_clamps_out_of_range_input() {
  TEST_ASSERT_EQUAL(0, dutyForPercent(-5));
  TEST_ASSERT_EQUAL(kMaxDuty, dutyForPercent(200));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_clamp_within_range_is_unchanged);
  RUN_TEST(test_clamp_below_zero);
  RUN_TEST(test_clamp_above_hundred);
  RUN_TEST(test_duty_zero_percent_is_off);
  RUN_TEST(test_duty_hundred_percent_is_max_duty);
  RUN_TEST(test_duty_default_fifteen_percent);
  RUN_TEST(test_duty_fifty_percent_rounds_up);
  RUN_TEST(test_duty_clamps_out_of_range_input);
  return UNITY_END();
}
