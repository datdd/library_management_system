#include "domain_core/user.h"
#include "domain_core/types.h"
#include "gtest/gtest.h"


using namespace lms::domain_core;

TEST(UserTest, ConstructorAndGetters) {
  User user("user1", "Alice Wonderland");
  EXPECT_EQ(user.getUserId(), "user1");
  EXPECT_EQ(user.getName(), "Alice Wonderland");
}

TEST(UserTest, SetName) {
  User user("user2", "Bob The Builder");
  user.setName("Robert The Builder");
  EXPECT_EQ(user.getName(), "Robert The Builder");
}

TEST(UserTest, ConstructorEmptyIdThrows) {
  EXPECT_THROW(User("", "Test User"), InvalidArgumentException);
}

TEST(UserTest, ConstructorEmptyNameThrows) {
  EXPECT_THROW(User("user3", ""), InvalidArgumentException);
}

TEST(UserTest, SetEmptyNameThrows) {
  User user("user4", "Initial Name");
  EXPECT_THROW(user.setName(""), InvalidArgumentException);
}