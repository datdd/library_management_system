#include "utils/date_time_utils.h"
#include <iostream>  // For debugging output if needed
#include <thread>    // For testing time differences briefly
#include "gtest/gtest.h"

using namespace lms::utils;
using namespace std::chrono_literals;

TEST(DateTimeUtilsTest, FormatDateTime) {
  // Create a specific time_point for consistent testing
  // Let's represent 2023-10-26 14:30:00
  std::tm t{};
  t.tm_year = 2023 - 1900;  // Years since 1900
  t.tm_mon = 10 - 1;        // Months since January (0-11)
  t.tm_mday = 26;           // Day of the month (1-31)
  t.tm_hour = 14;           // Hours since midnight (0-23)
  t.tm_min = 30;            // Minutes after the hour (0-59)
  t.tm_sec = 0;             // Seconds after the minute (0-59)
  t.tm_isdst = -1;          // Let mktime determine DST

  std::time_t time_t_val = std::mktime(&t);
  ASSERT_NE(time_t_val, -1);  // Ensure mktime succeeded
  DatePoint tp = std::chrono::system_clock::from_time_t(time_t_val);

  std::string formatted = DateTimeUtils::formatDateTime(tp);
  // The exact output can vary slightly by locale/system if not fully controlled,
  // but the core components should match.
  EXPECT_EQ(formatted, "2023-10-26 14:30:00");

  std::string formatted_date_only = DateTimeUtils::formatDate(tp);
  EXPECT_EQ(formatted_date_only, "2023-10-26");
}

TEST(DateTimeUtilsTest, ParseDate) {
  std::string date_str = "2023-11-15";
  std::optional<DatePoint> parsed_tp_opt = DateTimeUtils::parseDate(date_str);

  ASSERT_TRUE(parsed_tp_opt.has_value());
  DatePoint parsed_tp = parsed_tp_opt.value();

  // Verify by formatting back or comparing components
  // Parsing sets time to midnight.
  std::time_t parsed_time_t = std::chrono::system_clock::to_time_t(parsed_tp);
  std::tm t_parsed = *std::localtime(&parsed_time_t);

  EXPECT_EQ(t_parsed.tm_year, 2023 - 1900);
  EXPECT_EQ(t_parsed.tm_mon, 11 - 1);
  EXPECT_EQ(t_parsed.tm_mday, 15);
  EXPECT_EQ(t_parsed.tm_hour, 0);  // Should be midnight
  EXPECT_EQ(t_parsed.tm_min, 0);
  EXPECT_EQ(t_parsed.tm_sec, 0);

  // Test invalid date
  std::optional<DatePoint> invalid_opt = DateTimeUtils::parseDate("not-a-date");
  EXPECT_FALSE(invalid_opt.has_value());

  std::optional<DatePoint> invalid_fmt_opt =
      DateTimeUtils::parseDate("2023/11/15", "%Y-%m-%d");  // Wrong format
  EXPECT_FALSE(
      invalid_fmt_opt.has_value());  // std::get_time might be lenient; this depends on impl.

  std::optional<DatePoint> invalid_date_val_opt =
      DateTimeUtils::parseDate("2023-13-01");      // Invalid month
  EXPECT_FALSE(invalid_date_val_opt.has_value());  // mktime should catch this
}

TEST(DateTimeUtilsTest, AddDays) {
  std::optional<DatePoint> base_tp_opt = DateTimeUtils::parseDate("2023-10-20");
  ASSERT_TRUE(base_tp_opt.has_value());
  DatePoint base_tp = base_tp_opt.value();

  DatePoint future_tp = DateTimeUtils::addDays(base_tp, 5);
  std::string formatted_future = DateTimeUtils::formatDate(future_tp);
  EXPECT_EQ(formatted_future, "2023-10-25");

  DatePoint past_tp = DateTimeUtils::addDays(base_tp, -5);
  std::string formatted_past = DateTimeUtils::formatDate(past_tp);
  EXPECT_EQ(formatted_past, "2023-10-15");
}

TEST(DateTimeUtilsTest, NowAndToday) {
  DatePoint now_tp = DateTimeUtils::now();
  DatePoint today_tp = DateTimeUtils::today();

  // 'now' should be very recent
  auto diff_now = std::chrono::system_clock::now() - now_tp;
  EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(diff_now).count(), 2);

  // 'today' should have time components as 00:00:00
  std::time_t today_time_t_val = std::chrono::system_clock::to_time_t(today_tp);
  std::tm today_tm_val = *std::localtime(&today_time_t_val);

  EXPECT_EQ(today_tm_val.tm_hour, 0);
  EXPECT_EQ(today_tm_val.tm_min, 0);
  EXPECT_EQ(today_tm_val.tm_sec, 0);

  // 'today' should be less than or equal to 'now' on the same day
  EXPECT_LE(today_tp, now_tp);

  // Check if 'today' is indeed the start of the day for 'now'
  // This can be done by converting 'now' to its date components and comparing
  std::time_t now_for_compare_time_t = std::chrono::system_clock::to_time_t(now_tp);
  std::tm now_for_compare_tm = *std::localtime(&now_for_compare_time_t);

  EXPECT_EQ(today_tm_val.tm_year, now_for_compare_tm.tm_year);
  EXPECT_EQ(today_tm_val.tm_mon, now_for_compare_tm.tm_mon);
  EXPECT_EQ(today_tm_val.tm_mday, now_for_compare_tm.tm_mday);
}