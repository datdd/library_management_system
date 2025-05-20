#include "utils/date_time_utils.h"
#include <ctime>    // For std::tm, std::mktime
#include <iomanip>  // For std::put_time, std::get_time
#include <sstream>  // For std::stringstream

namespace lms {
namespace utils {

std::string DateTimeUtils::formatDateTime(const DatePoint& tp, const std::string& fmt) {
  std::time_t time_t_val = std::chrono::system_clock::to_time_t(tp);
  std::tm tm_val = *std::localtime(&time_t_val);  // Use localtime for local timezone representation
  // For UTC: std::tm tm_val = *std::gmtime(&time_t_val);

  std::stringstream ss;
  ss << std::put_time(&tm_val, fmt.c_str());
  return ss.str();
}

std::string DateTimeUtils::formatDate(const DatePoint& tp, const std::string& fmt) {
  // Same as formatDateTime for now, but could be specialized
  return formatDateTime(tp, fmt);
}

std::optional<DatePoint> DateTimeUtils::parseDate(const std::string& date_str,
                                                  const std::string& fmt) {
  std::tm tm_val = {};  // Zero-initialize
  std::stringstream ss(date_str);
  ss >> std::get_time(&tm_val, fmt.c_str());

  if (ss.fail()) {
    return std::nullopt;  // Parsing failed
  }

  // std::get_time might not fill all fields or might fill them inconsistently
  // if the format string is only for date. mktime normalizes them.
  // For a date-only string, time components (tm_hour, tm_min, tm_sec) might be 0 or garbage.
  // Set them to 0 for midnight.
  tm_val.tm_hour = 0;
  tm_val.tm_min = 0;
  tm_val.tm_sec = 0;
  tm_val.tm_isdst = -1;  // Let mktime determine DST

  std::time_t time_t_val = std::mktime(&tm_val);
  if (time_t_val == -1) {
    return std::nullopt;  // Invalid date components
  }

  return std::chrono::system_clock::from_time_t(time_t_val);
}

DatePoint DateTimeUtils::addDays(const DatePoint& tp, int days) {
  // Using C++20 `std::chrono::days` would be cleaner: `tp + std::chrono::days(days);`
  // For C++17, convert days to duration of seconds or hours.
  // `std::chrono::hours(24)` is a duration.
  return tp + std::chrono::hours(days * 24);
}

DatePoint DateTimeUtils::now() {
  return std::chrono::system_clock::now();
}

DatePoint DateTimeUtils::today() {
  auto now_tp = std::chrono::system_clock::now();
  std::time_t now_time_t = std::chrono::system_clock::to_time_t(now_tp);
  std::tm now_tm = *std::localtime(&now_time_t);  // or gmtime for UTC

  // Zero out time components
  now_tm.tm_hour = 0;
  now_tm.tm_min = 0;
  now_tm.tm_sec = 0;
  // tm_isdst = -1 to let mktime figure it out, or set explicitly if you know
  now_tm.tm_isdst = -1;

  std::time_t today_time_t = std::mktime(&now_tm);
  return std::chrono::system_clock::from_time_t(today_time_t);
}

}  // namespace utils
}  // namespace lms