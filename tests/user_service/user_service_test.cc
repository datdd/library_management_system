#include "user_service/user_service.h"
#include "domain_core/types.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mock_persistence_service.h"

using namespace lms::user_service;
using namespace lms::domain_core;
using namespace lms::testing;

// For GMock actions and matchers
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Throw;

class UserServiceTest : public ::testing::Test {
protected:
  std::shared_ptr<NiceMock<MockPersistenceService>> mock_persistence_service_;
  // Use IUserService for interface testing if preferred
  std::unique_ptr<UserService> user_service_;

  UserServiceTest() {}  // To initialize members in SetUp or below

  void SetUp() override {
    mock_persistence_service_ = std::make_shared<NiceMock<MockPersistenceService>>();
    user_service_ = std::make_unique<UserService>(mock_persistence_service_);
  }
};

TEST_F(UserServiceTest, AddUserSuccessfully) {
  EntityId user_id = "user123";
  std::string name = "John Doe";
  User expected_user(user_id, name);

  // Expect loadUser to find no existing user
  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id)).WillOnce(Return(std::nullopt));
  // Expect saveUser to be called with the correct User object
  EXPECT_CALL(*mock_persistence_service_, saveUser(expected_user)).Times(1);

  ASSERT_NO_THROW(user_service_->addUser(user_id, name));
}

TEST_F(UserServiceTest, AddUserFailsIfUserExists) {
  EntityId user_id = "user123";
  std::string name = "John Doe";
  User existing_user(user_id, name);

  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id))
      .WillOnce(Return(existing_user));  // Simulate user already exists

  EXPECT_THROW(user_service_->addUser(user_id, name), OperationFailedException);
}

TEST_F(UserServiceTest, AddUserFailsWithEmptyId) {
  EXPECT_THROW(user_service_->addUser("", "John Doe"), InvalidArgumentException);
}

TEST_F(UserServiceTest, AddUserFailsWithEmptyName) {
  EXPECT_THROW(user_service_->addUser("user123", ""), InvalidArgumentException);
}

TEST_F(UserServiceTest, FindUserByIdSuccessfully) {
  EntityId user_id = "user123";
  User expected_user(user_id, "Jane Doe");

  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id)).WillOnce(Return(expected_user));

  auto found_user_opt = user_service_->findUserById(user_id);
  ASSERT_TRUE(found_user_opt.has_value());
  EXPECT_EQ(found_user_opt.value().getUserId(), user_id);
  EXPECT_EQ(found_user_opt.value().getName(), "Jane Doe");
}

TEST_F(UserServiceTest, FindUserByIdReturnsNulloptIfNotFound) {
  EntityId user_id = "nonexistent";
  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id)).WillOnce(Return(std::nullopt));

  auto found_user_opt = user_service_->findUserById(user_id);
  EXPECT_FALSE(found_user_opt.has_value());
}

TEST_F(UserServiceTest, FindUserByIdThrowsOnEmptyId) {
  EXPECT_THROW(user_service_->findUserById(""), InvalidArgumentException);
}

TEST_F(UserServiceTest, GetAllUsers) {
  std::vector<User> expected_users = {User("user1", "Alice"), User("user2", "Bob")};
  EXPECT_CALL(*mock_persistence_service_, loadAllUsers()).WillOnce(Return(expected_users));

  auto all_users = user_service_->getAllUsers();
  ASSERT_EQ(all_users.size(), 2);
  EXPECT_EQ(all_users[0].getName(), "Alice");
  EXPECT_EQ(all_users[1].getName(), "Bob");
}

TEST_F(UserServiceTest, FindUsersByName) {
  std::vector<User> all_users_from_db = {User("user1", "Charlie Brown"),
                                         User("user2", "Sally Brown"),
                                         User("user3", "Charlie Chaplin")};
  std::string search_name = "Charlie Brown";
  EXPECT_CALL(*mock_persistence_service_, loadAllUsers()).WillOnce(Return(all_users_from_db));

  auto found_users = user_service_->findUsersByName(search_name);
  ASSERT_EQ(found_users.size(), 1);
  EXPECT_EQ(found_users[0].getUserId(), "user1");
}

TEST_F(UserServiceTest, FindUsersByNameReturnsEmptyIfNoneMatch) {
  std::vector<User> all_users_from_db = {User("user1", "Alpha")};
  EXPECT_CALL(*mock_persistence_service_, loadAllUsers()).WillOnce(Return(all_users_from_db));

  auto found_users = user_service_->findUsersByName("Beta");
  EXPECT_TRUE(found_users.empty());
}

TEST_F(UserServiceTest, UpdateUserSuccessfully) {
  EntityId user_id = "userEdit";
  std::string old_name = "Old Name";
  std::string new_name = "New Name";
  User original_user(user_id, old_name);
  User updated_user(user_id, new_name);  // The state after update

  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id)).WillOnce(Return(original_user));
  EXPECT_CALL(*mock_persistence_service_, saveUser(updated_user))  // Expect save with new name
      .Times(1);

  ASSERT_NO_THROW(user_service_->updateUser(user_id, new_name));
}

TEST_F(UserServiceTest, UpdateUserFailsIfNotFound) {
  EntityId user_id = "nonexistentEdit";
  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id)).WillOnce(Return(std::nullopt));

  EXPECT_THROW(user_service_->updateUser(user_id, "Any Name"), NotFoundException);
}

TEST_F(UserServiceTest, UpdateUserFailsWithEmptyNewName) {
  EntityId user_id = "userEdit";
  // No need to mock loadUser if the validation fails before that
  EXPECT_THROW(user_service_->updateUser(user_id, ""), InvalidArgumentException);
}

TEST_F(UserServiceTest, RemoveUserSuccessfully) {
  EntityId user_id = "userRemove";
  User existing_user(user_id, "ToRemove");

  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id))  // To check existence
      .WillOnce(Return(existing_user));
  EXPECT_CALL(*mock_persistence_service_, deleteUser(user_id)).Times(1);

  EXPECT_TRUE(user_service_->removeUser(user_id));
}

TEST_F(UserServiceTest, RemoveUserReturnsFalseIfNotFound) {
  EntityId user_id = "nonexistentRemove";
  EXPECT_CALL(*mock_persistence_service_, loadUser(user_id)).WillOnce(Return(std::nullopt));
  // deleteUser should not be called if loadUser returns nullopt
  EXPECT_CALL(*mock_persistence_service_, deleteUser(user_id)).Times(0);

  EXPECT_FALSE(user_service_->removeUser(user_id));
}

TEST_F(UserServiceTest, RemoveUserThrowsOnEmptyId) {
  EXPECT_THROW(user_service_->removeUser(""), InvalidArgumentException);
}