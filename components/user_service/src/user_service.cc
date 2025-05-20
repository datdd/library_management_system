#include "user_service/user_service.h"

#include "domain_core/types.h"  // For exceptions

namespace lms {
namespace user_service {

UserService::UserService(
    std::shared_ptr<persistence_service::IPersistenceService> persistence_service)
    : m_persistence_service(std::move(persistence_service)) {
  if (!m_persistence_service) {
    throw std::invalid_argument("Persistence service cannot be null for UserService.");
  }
}

void UserService::addUser(const EntityId& user_id, const std::string& name) {
  // Basic validation for id and name (domain_core::User constructor also
  // validates)
  if (user_id.empty()) {
    throw InvalidArgumentException("User ID cannot be empty for addUser.");
  }
  if (name.empty()) {
    throw InvalidArgumentException("User name cannot be empty for addUser.");
  }

  // Check if user already exists
  if (m_persistence_service->loadUser(user_id).has_value()) {
    throw OperationFailedException("User with ID '" + user_id + "' already exists.");
  }

  // User constructor will throw if args are invalid per its rules
  User new_user(user_id, name);
  m_persistence_service->saveUser(new_user);
}

std::optional<User> UserService::findUserById(const EntityId& user_id) const {
  if (user_id.empty()) {
    // Or return std::nullopt silently
    throw InvalidArgumentException("User ID cannot be empty for findUserById.");
  }
  return m_persistence_service->loadUser(user_id);
}

std::vector<User> UserService::findUsersByName(const std::string& name) const {
  if (name.empty()) {
    throw InvalidArgumentException("User name cannot be empty for findUsersByName.");
  }
  std::vector<User> all_users = m_persistence_service->loadAllUsers();
  std::vector<User> matching_users;
  for (const auto& user : all_users) {
    if (user.getName() == name) {
      matching_users.push_back(user);
    }
  }
  return matching_users;
}

std::vector<User> UserService::getAllUsers() const {
  return m_persistence_service->loadAllUsers();
}

void UserService::updateUser(const EntityId& user_id, const std::string& new_name) {
  if (user_id.empty()) {
    throw InvalidArgumentException("User ID cannot be empty for updateUser.");
  }
  if (new_name.empty()) {
    throw InvalidArgumentException("New user name cannot be empty for updateUser.");
  }

  auto user_opt = m_persistence_service->loadUser(user_id);
  if (!user_opt.has_value()) {
    throw NotFoundException("User with ID '" + user_id + "' not found for update.");
  }

  User user_to_update = user_opt.value();
  user_to_update.setName(new_name);  // User::setName will validate new_name
  m_persistence_service->saveUser(
      user_to_update);  // saveUser will overwrite due to insert_or_assign
}

bool UserService::removeUser(const EntityId& user_id) {
  if (user_id.empty()) {
    throw InvalidArgumentException("User ID cannot be empty for removeUser.");
  }
  // First check if user exists to return accurate boolean,
  // though deleteUser in persistence might be idempotent.
  if (!m_persistence_service->loadUser(user_id).has_value()) {
    return false;
  }
  m_persistence_service->deleteUser(user_id);
  return true;  // Assuming deleteUser succeeds if it doesn't throw
}

}  // namespace user_service
}  // namespace lms