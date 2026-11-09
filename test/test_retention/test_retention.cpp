#include <unity.h>

#include <vector>

#include "retention.h"

using Retention::DayFileInfo;
using Retention::filesToDelete;

void test_no_deletion_when_enough_free_space() {
  std::vector<DayFileInfo> days = {{"2026-11-01.csv", 1000}, {"2026-11-02.csv", 1000}};
  auto victims = filesToDelete(days, 5000, 1000, "2026-11-02.csv");
  TEST_ASSERT_EQUAL(0, victims.size());
}

void test_deletes_oldest_first() {
  std::vector<DayFileInfo> days = {
      {"2026-11-03.csv", 500},
      {"2026-11-01.csv", 500},
      {"2026-11-02.csv", 500},
  };
  auto victims = filesToDelete(days, 100, 600, "2026-11-03.csv");
  TEST_ASSERT_EQUAL(1, victims.size());
  TEST_ASSERT_EQUAL_STRING("2026-11-01.csv", victims[0].c_str());
}

void test_deletes_multiple_until_enough_space() {
  std::vector<DayFileInfo> days = {
      {"2026-11-01.csv", 300},
      {"2026-11-02.csv", 300},
      {"2026-11-03.csv", 300},
      {"2026-11-04.csv", 300},
  };
  auto victims = filesToDelete(days, 100, 900, "2026-11-04.csv");
  TEST_ASSERT_EQUAL(3, victims.size());
  TEST_ASSERT_EQUAL_STRING("2026-11-01.csv", victims[0].c_str());
  TEST_ASSERT_EQUAL_STRING("2026-11-02.csv", victims[1].c_str());
  TEST_ASSERT_EQUAL_STRING("2026-11-03.csv", victims[2].c_str());
}

void test_never_deletes_current_day_file() {
  std::vector<DayFileInfo> days = {{"2026-11-04.csv", 300}};
  auto victims = filesToDelete(days, 0, 1000, "2026-11-04.csv");
  TEST_ASSERT_EQUAL(0, victims.size());
}

void test_stops_once_target_reached_without_extra_deletion() {
  std::vector<DayFileInfo> days = {
      {"2026-11-01.csv", 1000},
      {"2026-11-02.csv", 1000},
  };
  auto victims = filesToDelete(days, 400, 1000, "2026-11-03.csv");
  TEST_ASSERT_EQUAL(1, victims.size());
  TEST_ASSERT_EQUAL_STRING("2026-11-01.csv", victims[0].c_str());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_no_deletion_when_enough_free_space);
  RUN_TEST(test_deletes_oldest_first);
  RUN_TEST(test_deletes_multiple_until_enough_space);
  RUN_TEST(test_never_deletes_current_day_file);
  RUN_TEST(test_stops_once_target_reached_without_extra_deletion);
  return UNITY_END();
}
