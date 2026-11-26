#include <unity.h>

#include <cstring>

#include "run_log.h"

using RunLog::escapeField;
using RunLog::formatEndLine;
using RunLog::formatStartLine;
using RunLog::StartFields;

void test_escape_field_plain_text_unchanged() {
  char buf[32];
  escapeField("proef-3", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("proef-3", buf);
}

void test_escape_field_comma_gets_quoted() {
  char buf[32];
  escapeField("bloem, water", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("\"bloem, water\"", buf);
}

void test_escape_field_quote_gets_doubled_and_quoted() {
  char buf[32];
  escapeField("a \"wet\" starter", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("\"a \"\"wet\"\" starter\"", buf);
}

void test_escape_field_newline_gets_quoted() {
  char buf[32];
  escapeField("line one\nline two", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("\"line one\nline two\"", buf);
}

void test_escape_field_empty_string() {
  char buf[8];
  escapeField("", buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING("", buf);
}

void test_escape_field_truncates_without_overflow() {
  char buf[5];
  escapeField("abcdefgh", buf, sizeof(buf));
  TEST_ASSERT_EQUAL(4u, strlen(buf));
  TEST_ASSERT_EQUAL_STRING("abcd", buf);
}

void test_format_start_line_plain_fields() {
  StartFields fields;
  fields.epoch = 1700000000;
  fields.name = "proef-3";
  fields.product = "zuurdesem";
  fields.starterG = 20.0f;
  fields.flourG = 20.0f;
  fields.waterG = 20.0f;
  fields.waterTempC = 22.5f;
  fields.note = "";

  char buf[320];
  TEST_ASSERT_TRUE(formatStartLine(fields, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("1700000000,start,proef-3,zuurdesem,20.0,20.0,20.0,22.5,", buf);
}

void test_format_start_line_escapes_note_with_comma() {
  StartFields fields;
  fields.epoch = 1700000000;
  fields.name = "proef-4";
  fields.product = "zuurdesem";
  fields.starterG = 10.0f;
  fields.flourG = 10.0f;
  fields.waterG = 10.0f;
  fields.waterTempC = 21.0f;
  fields.note = "warm, in de oven";

  char buf[320];
  TEST_ASSERT_TRUE(formatStartLine(fields, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("1700000000,start,proef-4,zuurdesem,10.0,10.0,10.0,21.0,\"warm, in de oven\"",
                            buf);
}

void test_format_start_line_too_small_buffer_fails() {
  StartFields fields;
  fields.epoch = 1700000000;
  fields.name = "proef-5";
  fields.product = "zuurdesem";
  fields.starterG = 10.0f;
  fields.flourG = 10.0f;
  fields.waterG = 10.0f;
  fields.waterTempC = 21.0f;
  fields.note = "note";

  char buf[8];
  TEST_ASSERT_FALSE(formatStartLine(fields, buf, sizeof(buf)));
}

void test_format_end_line_leaves_numeric_fields_empty() {
  char buf[320];
  TEST_ASSERT_TRUE(formatEndLine(1700003600, "proef-3", "klaar", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("1700003600,end,proef-3,,,,,,klaar", buf);
}

void test_format_end_line_escapes_name_with_quote() {
  char buf[320];
  TEST_ASSERT_TRUE(formatEndLine(1700003600, "proef \"special\"", "", buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_STRING("1700003600,end,\"proef \"\"special\"\"\",,,,,,", buf);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_escape_field_plain_text_unchanged);
  RUN_TEST(test_escape_field_comma_gets_quoted);
  RUN_TEST(test_escape_field_quote_gets_doubled_and_quoted);
  RUN_TEST(test_escape_field_newline_gets_quoted);
  RUN_TEST(test_escape_field_empty_string);
  RUN_TEST(test_escape_field_truncates_without_overflow);
  RUN_TEST(test_format_start_line_plain_fields);
  RUN_TEST(test_format_start_line_escapes_note_with_comma);
  RUN_TEST(test_format_start_line_too_small_buffer_fails);
  RUN_TEST(test_format_end_line_leaves_numeric_fields_empty);
  RUN_TEST(test_format_end_line_escapes_name_with_quote);
  return UNITY_END();
}
