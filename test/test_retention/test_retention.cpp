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

void test_min_free_bytes_is_500_mb() {
  TEST_ASSERT_EQUAL_UINT64(500ULL * 1024 * 1024, Retention::kMinFreeBytes);
}

// F2: filesToDelete is called with /photos/YYYY-MM-DD day directory names
// (no .csv suffix) and SD-card-scale free space, which overflows uint32_t.
void test_deletes_oldest_photo_day_directory_at_card_scale() {
  std::vector<DayFileInfo> days = {
      {"2026-11-07", 2ULL * 1024 * 1024 * 1024},
      {"2026-11-08", 2ULL * 1024 * 1024 * 1024},
      {"2026-11-09", 2ULL * 1024 * 1024 * 1024},
  };
  uint64_t freeBytes = 100ULL * 1024 * 1024;  // below the 500 MB floor
  auto victims = filesToDelete(days, freeBytes, Retention::kMinFreeBytes, "2026-11-09");
  TEST_ASSERT_EQUAL(1, victims.size());
  TEST_ASSERT_EQUAL_STRING("2026-11-07", victims[0].c_str());
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_no_deletion_when_enough_free_space);
  RUN_TEST(test_deletes_oldest_first);
  RUN_TEST(test_deletes_multiple_until_enough_space);
  RUN_TEST(test_never_deletes_current_day_file);
  RUN_TEST(test_stops_once_target_reached_without_extra_deletion);
  RUN_TEST(test_min_free_bytes_is_500_mb);
  RUN_TEST(test_deletes_oldest_photo_day_directory_at_card_scale);
  return UNITY_END();
}
