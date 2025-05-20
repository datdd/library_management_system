#include "domain_core/book.h"
#include "domain_core/author.h"
#include "domain_core/types.h"  // For InvalidArgumentException
#include "gtest/gtest.h"


using namespace lms::domain_core;

class BookTest : public ::testing::Test {
protected:
  std::shared_ptr<Author> test_author;
  void SetUp() override { test_author = std::make_shared<Author>("auth1", "Test Author"); }
};

TEST_F(BookTest, ConstructorAndGetters) {
  Book book("book1", "Test Book", test_author, "1234567890", 2023, AvailabilityStatus::AVAILABLE);
  EXPECT_EQ(book.getId(), "book1");
  EXPECT_EQ(book.getTitle(), "Test Book");
  ASSERT_NE(book.getAuthor(), nullptr);
  EXPECT_EQ(book.getAuthor()->getId(), "auth1");
  EXPECT_EQ(book.getIsbn(), "1234567890");
  EXPECT_EQ(book.getPublicationYear(), 2023);
  EXPECT_EQ(book.getAvailabilityStatus(), AvailabilityStatus::AVAILABLE);
}

TEST_F(BookTest, Setters) {
  Book book("book2", "Old Title", test_author, "0987654321", 2020);
  book.setTitle("New Title");
  EXPECT_EQ(book.getTitle(), "New Title");

  auto new_author = std::make_shared<Author>("auth2", "New Author");
  book.setAuthor(new_author);
  ASSERT_NE(book.getAuthor(), nullptr);
  EXPECT_EQ(book.getAuthor()->getName(), "New Author");

  book.setPublicationYear(2021);
  EXPECT_EQ(book.getPublicationYear(), 2021);

  book.setIsbn("1122334455");
  EXPECT_EQ(book.getIsbn(), "1122334455");

  book.setAvailabilityStatus(AvailabilityStatus::BORROWED);
  EXPECT_EQ(book.getAvailabilityStatus(), AvailabilityStatus::BORROWED);
}

TEST_F(BookTest, ConstructorValidations) {
  EXPECT_THROW(Book("", "Title", test_author, "isbn", 2000),
               InvalidArgumentException);  // Empty ID
  EXPECT_THROW(Book("b1", "", test_author, "isbn", 2000),
               InvalidArgumentException);  // Empty Title
  EXPECT_THROW(Book("b1", "Title", nullptr, "isbn", 2000),
               InvalidArgumentException);  // Null Author
  EXPECT_THROW(Book("b1", "Title", test_author, "", 2000),
               InvalidArgumentException);  // Empty ISBN
  EXPECT_THROW(Book("b1", "Title", test_author, "isbn", 0),
               InvalidArgumentException);  // Invalid Year
}