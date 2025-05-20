#include "domain_core/author.h"
#include "domain_core/types.h"  // For InvalidArgumentException
#include "gtest/gtest.h"

using namespace lms::domain_core;

TEST(AuthorTest, ConstructorAndGetters) {
  Author author("author1", "John Doe");
  EXPECT_EQ(author.getId(), "author1");
  EXPECT_EQ(author.getName(), "John Doe");
}

TEST(AuthorTest, SetName) {
  Author author("author2", "Jane Doe");
  author.setName("Jane Smith");
  EXPECT_EQ(author.getName(), "Jane Smith");
}

TEST(AuthorTest, ConstructorEmptyIdThrows) {
  EXPECT_THROW(Author("", "John Doe"), InvalidArgumentException);
}

TEST(AuthorTest, ConstructorEmptyNameThrows) {
  EXPECT_THROW(Author("author3", ""), InvalidArgumentException);
}

TEST(AuthorTest, SetEmptyNameThrows) {
  Author author("author4", "Test Name");
  EXPECT_THROW(author.setName(""), InvalidArgumentException);
}