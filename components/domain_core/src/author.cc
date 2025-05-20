#include "domain_core/author.h"

#include "domain_core/types.h"  // For InvalidArgumentException

namespace lms {
namespace domain_core {

Author::Author(EntityId id, std::string name) : m_id(std::move(id)), m_name(std::move(name)) {
  if (m_id.empty()) {
    throw InvalidArgumentException("Author ID cannot be empty.");
  }
  if (m_name.empty()) {
    throw InvalidArgumentException("Author name cannot be empty.");
  }
}

const EntityId& Author::getId() const {
  return m_id;
}

const std::string& Author::getName() const {
  return m_name;
}

void Author::setName(std::string name) {
  if (name.empty()) {
    throw InvalidArgumentException("Author name cannot be empty.");
  }
  m_name = std::move(name);
}

bool Author::operator==(const Author& other) const {
  return m_id == other.m_id && m_name == other.m_name;
}

bool Author::operator!=(const Author& other) const {
  return !(*this == other);
}

}  // namespace domain_core
}  // namespace lms