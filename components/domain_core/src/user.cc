#include "domain_core/user.h"

#include "domain_core/types.h"  // For InvalidArgumentException

namespace lms {
namespace domain_core {

User::User(EntityId user_id, std::string name)
    : m_user_id(std::move(user_id)), m_name(std::move(name)) {
  if (m_user_id.empty()) {
    throw InvalidArgumentException("User ID cannot be empty.");
  }
  if (m_name.empty()) {
    throw InvalidArgumentException("User name cannot be empty.");
  }
}

const EntityId& User::getUserId() const {
  return m_user_id;
}

const std::string& User::getName() const {
  return m_name;
}

void User::setName(std::string name) {
  if (name.empty()) {
    throw InvalidArgumentException("User name cannot be empty.");
  }
  m_name = std::move(name);
}

bool User::operator==(const User& other) const {
  return m_user_id == other.m_user_id && m_name == other.m_name;
}

bool User::operator!=(const User& other) const {
  return !(*this == other);
}

}  // namespace domain_core
}  // namespace lms