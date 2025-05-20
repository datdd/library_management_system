#ifndef LMS_USER_SERVICE_I_USER_SERVICE_H
#define LMS_USER_SERVICE_I_USER_SERVICE_H

#include "domain_core/types.h"  // For EntityId, std::optional, exceptions
#include "domain_core/user.h"

#include <optional>
#include <string>
#include <vector>

namespace lms {
namespace user_service {

using namespace domain_core;  // Make domain types easily accessible

class IUserService {
public:
  virtual ~IUserService() = default;

  // Adds a new user to the system.
  // Throws InvalidArgumentException if user data is invalid.
  // Throws OperationFailedException if a user with the same ID already exists.
  virtual void addUser(const EntityId& user_id, const std::string& name) = 0;

  // Finds a user by their ID.
  // Returns std::nullopt if no user is found with the given ID.
  virtual std::optional<User> findUserById(const EntityId& user_id) const = 0;

  // Finds users by name (can return multiple users if names are not unique).
  // For simplicity, let's assume it returns users with exact name match.
  virtual std::vector<User> findUsersByName(const std::string& name) const = 0;

  // Retrieves all users in the system.
  virtual std::vector<User> getAllUsers() const = 0;

  // Updates an existing user's information (e.g., name).
  // Throws NotFoundException if the user_id does not exist.
  // Throws InvalidArgumentException if the new name is invalid.
  virtual void updateUser(const EntityId& user_id, const std::string& new_name) = 0;

  // Removes a user from the system.
  // Returns true if a user was removed, false if no user with that ID was found.
  virtual bool removeUser(const EntityId& user_id) = 0;
};

}  // namespace user_service
}  // namespace lms

#endif  // LMS_USER_SERVICE_I_USER_SERVICE_H