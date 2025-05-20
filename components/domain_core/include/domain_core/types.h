#ifndef LMS_DOMAIN_CORE_TYPES_H
#define LMS_DOMAIN_CORE_TYPES_H

#include <chrono>
#include <optional>   // For std::optional
#include <stdexcept>  // For custom exceptions
#include <string>

namespace lms {
namespace domain_core {

using EntityId = std::string;
using Date = std::chrono::system_clock::time_point;

// Base exception class for the LMS
class LmsException : public std::runtime_error {
public:
  explicit LmsException(const std::string& message) : std::runtime_error(message) {}
};

class NotFoundException : public LmsException {
public:
  explicit NotFoundException(const std::string& message) : LmsException(message) {}
};

class InvalidArgumentException : public LmsException {
public:
  explicit InvalidArgumentException(const std::string& message) : LmsException(message) {}
};

class OperationFailedException : public LmsException {
public:
  explicit OperationFailedException(const std::string& message) : LmsException(message) {}
};

}  // namespace domain_core
}  // namespace lms

#endif  // LMS_DOMAIN_CORE_TYPES_H