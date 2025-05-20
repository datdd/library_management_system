#ifndef LMS_TESTS_MOCKS_MOCK_USER_SERVICE_H
#define LMS_TESTS_MOCKS_MOCK_USER_SERVICE_H

#include "gmock/gmock.h"
#include "user_service/i_user_service.h"

namespace lms {
namespace testing {
using namespace user_service;
class MockUserService : public IUserService {
public:
  MOCK_METHOD(void, addUser, (const EntityId& user_id, const std::string& name), (override));
  MOCK_METHOD(std::optional<User>, findUserById, (const EntityId& user_id), (const, override));
  MOCK_METHOD(std::vector<User>, findUsersByName, (const std::string& name), (const, override));
  MOCK_METHOD(std::vector<User>, getAllUsers, (), (const, override));
  MOCK_METHOD(void, updateUser, (const EntityId& user_id, const std::string& new_name), (override));
  MOCK_METHOD(bool, removeUser, (const EntityId& user_id), (override));
};
}  // namespace testing
}  // namespace lms
#endif