#include <unity.h>

#include <cstring>

#include "status_line.h"

using StatusLine::Counters;
using StatusLine::formatLine;
using StatusLine::Snapshot;

void test_format_line_with_all_counters_ok() {
  Snapshot s = {1731400000, "0.6.0", 3661, {59, 1, 4, 0}, 14234, 238632, 3973123, ""};
  char buf[220];
  formatLine(s, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("1731400000,0.6.0,3661,59,1,4,0,14234,238632,3973123,", buf);
}

void test_format_line_with_last_error() {
  Snapshot s = {1731403600, "0.6.0", 7261, {60, 0, 4, 1}, 14230, 240000, 3970000,
                "camera error: no frame"};
  char buf[220];
  formatLine(s, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
      "1731403600,0.6.0,7261,60,0,4,1,14230,240000,3970000,camera error: no frame", buf);
}

void test_format_line_truncates_to_buffer_size() {
  Snapshot s = {1731400000, "0.6.0", 1, {0, 0, 0, 0}, 0, 0, 0, "some very long error message"};
  char buf[16];
  formatLine(s, buf, sizeof(buf));
  TEST_ASSERT_EQUAL(15u, strlen(buf));  // snprintf always null-terminates within bufSize.
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_format_line_with_all_counters_ok);
  RUN_TEST(test_format_line_with_last_error);
  RUN_TEST(test_format_line_truncates_to_buffer_size);
  return UNITY_END();
}
