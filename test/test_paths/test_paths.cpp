#include <unity.h>

#include <ctime>

#include "paths.h"

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

void test_measurement_csv_path() {
  char buf[32];
  Paths::measurementCsvPath(utc(2026, 11, 9, 14, 5, 30), buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("/data/2026-11-09.csv", buf);
}

void test_photo_dir_path() {
  char buf[24];
  Paths::photoDirPath(utc(2026, 11, 9, 14, 5, 30), buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("/photos/2026-11-09", buf);
}

void test_photo_file_path() {
  char buf[40];
  Paths::photoFilePath(utc(2026, 11, 9, 14, 5, 30), buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("/photos/2026-11-09/140530.jpg", buf);
}

void test_paths_zero_pad_single_digit_values() {
  char csvBuf[32];
  char photoBuf[40];
  Paths::measurementCsvPath(utc(2026, 1, 2, 3, 4, 5), csvBuf, sizeof(csvBuf));
  Paths::photoFilePath(utc(2026, 1, 2, 3, 4, 5), photoBuf, sizeof(photoBuf));
  TEST_ASSERT_EQUAL_STRING("/data/2026-01-02.csv", csvBuf);
  TEST_ASSERT_EQUAL_STRING("/photos/2026-01-02/030405.jpg", photoBuf);
}

void test_photo_file_path_shares_day_with_dir_path() {
  time_t t = utc(2026, 12, 31, 23, 59, 59);
  char dirBuf[24];
  char fileBuf[40];
  Paths::photoDirPath(t, dirBuf, sizeof(dirBuf));
  Paths::photoFilePath(t, fileBuf, sizeof(fileBuf));
  TEST_ASSERT_EQUAL_STRING("/photos/2026-12-31", dirBuf);
  TEST_ASSERT_EQUAL_STRING("/photos/2026-12-31/235959.jpg", fileBuf);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_measurement_csv_path);
  RUN_TEST(test_photo_dir_path);
  RUN_TEST(test_photo_file_path);
  RUN_TEST(test_paths_zero_pad_single_digit_values);
  RUN_TEST(test_photo_file_path_shares_day_with_dir_path);
  return UNITY_END();
}
