#ifndef LMS_USER_SERVICE_USER_SERVICE_H
#define LMS_USER_SERVICE_USER_SERVICE_H

#include <memory>  // For std::shared_ptr for persistence service
#include <mutex>   // For thread safety if caching or internal state management
#include "i_user_service.h"
#include "persistence_service/i_persistence_service.h"  // Dependency

namespace lms {
namespace user_service {

class UserService : public IUserService {
public:
  // UserService depends on an IPersistenceService implementation.
  explicit UserService(
      std::shared_ptr<persistence_service::IPersistenceService> persistence_service);
  ~UserService() override = default;

  void addUser(const EntityId& user_id, const std::string& name) override;
  std::optional<User> findUserById(const EntityId& user_id) const override;
  std::vector<User> findUsersByName(const std::string& name) const override;
  std::vector<User> getAllUsers() const override;
  void updateUser(const EntityId& user_id, const std::string& new_name) override;
  bool removeUser(const EntityId& user_id) override;

private:
  std::shared_ptr<persistence_service::IPersistenceService> m_persistence_service;
  // mutable std::mutex m_mutex; // If we had internal caching specific to UserService
  // For now, persistence service handles its own thread safety.
};

}  // namespace user_service
}  // namespace lms

#endif  // LMS_USER_SERVICE_USER_SERVICE_H