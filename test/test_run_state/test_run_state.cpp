#include <unity.h>

#include <cstring>

#include "run_state.h"

using RunState::current;
using RunState::elapsedSeconds;
using RunState::end;

void setUp() {
  end();  // Reset the module-level state before every test.
}

void test_no_run_active_by_default() {
  RunState::Current c = current();
  TEST_ASSERT_FALSE(c.active);
  TEST_ASSERT_EQUAL(0, elapsedSeconds(1700000000));
}

void test_start_makes_it_active_with_given_name() {
  RunState::start("proef-3", 1700000000);
  RunState::Current c = current();
  TEST_ASSERT_TRUE(c.active);
  TEST_ASSERT_EQUAL_STRING("proef-3", c.name);
  TEST_ASSERT_EQUAL(1700000000, c.startEpoch);
}

void test_elapsed_seconds_counts_from_start() {
  RunState::start("proef-3", 1700000000);
  TEST_ASSERT_EQUAL(3600, elapsedSeconds(1700003600));
}

void test_elapsed_seconds_zero_before_start_time() {
  RunState::start("proef-3", 1700000000);
  TEST_ASSERT_EQUAL(0, elapsedSeconds(1699999000));
}

void test_end_clears_active_run() {
  RunState::start("proef-3", 1700000000);
  RunState::end();
  RunState::Current c = current();
  TEST_ASSERT_FALSE(c.active);
  TEST_ASSERT_EQUAL(0, elapsedSeconds(1700003600));
}

void test_starting_a_new_run_overwrites_the_previous_one() {
  RunState::start("proef-3", 1700000000);
  RunState::start("proef-4", 1700010000);
  RunState::Current c = current();
  TEST_ASSERT_TRUE(c.active);
  TEST_ASSERT_EQUAL_STRING("proef-4", c.name);
  TEST_ASSERT_EQUAL(1700010000, c.startEpoch);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_no_run_active_by_default);
  RUN_TEST(test_start_makes_it_active_with_given_name);
  RUN_TEST(test_elapsed_seconds_counts_from_start);
  RUN_TEST(test_elapsed_seconds_zero_before_start_time);
  RUN_TEST(test_end_clears_active_run);
  RUN_TEST(test_starting_a_new_run_overwrites_the_previous_one);
  return UNITY_END();
}
