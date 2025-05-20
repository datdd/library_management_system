#ifndef LMS_UTILS_DATE_TIME_UTILS_H
#define LMS_UTILS_DATE_TIME_UTILS_H

#include <chrono>
#include <optional>  // For parsing optional time
#include <string>

namespace lms {
namespace utils {

// Alias for clarity within this utility
using DatePoint = std::chrono::system_clock::time_point;

class DateTimeUtils {
public:
  // Formats a time_point to a string (e.g., "YYYY-MM-DD HH:MM:SS")
  static std::string formatDateTime(const DatePoint& tp,
                                    const std::string& fmt = "%Y-%m-%d %H:%M:%S");

  // Formats a time_point to a date string (e.g., "YYYY-MM-DD")
  static std::string formatDate(const DatePoint& tp, const std::string& fmt = "%Y-%m-%d");

  // Parses a date string (e.g., "YYYY-MM-DD") into a time_point
  // Returns std::nullopt if parsing fails or input is invalid.
  // Time part will be set to midnight of that day.
  static std::optional<DatePoint> parseDate(const std::string& date_str,
                                            const std::string& fmt = "%Y-%m-%d");

  // Adds a specified number of days to a time_point
  static DatePoint addDays(const DatePoint& tp, int days);

  // Get current date and time
  static DatePoint now();

  // Get today's date (time part set to midnight)
  static DatePoint today();
};

}  // namespace utils
}  // namespace lms

#endif  // LMS_UTILS_DATE_TIME_UTILS_H