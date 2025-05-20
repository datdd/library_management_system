#include "persistence_service/in_memory_persistence_service.h"
#include "domain_core/author.h"
#include "domain_core/book.h"  // For testing with a concrete ILibraryItem
#include "domain_core/loan_record.h"
#include "domain_core/user.h"
#include "gtest/gtest.h"
#include "utils/date_time_utils.h"  // For dates

#include <memory>

using namespace lms::persistence_service;
using namespace lms::domain_core;
using namespace lms::utils;  // For DateTimeUtils

class InMemoryPersistenceServiceTest : public ::testing::Test {
protected:
  InMemoryPersistenceService service;
  std::shared_ptr<Author> author1;
  std::unique_ptr<Book> book1;  // Use unique_ptr for the original item
  User user1;
  LoanRecord loan1;

  InMemoryPersistenceServiceTest()
      : user1("dummy_user_id", "Dummy User"),
        loan1("dummy_loan_id",
              "dummy_item_id",
              "dummy_user_id",
              DateTimeUtils::today(),
              DateTimeUtils::addDays(DateTimeUtils::today(), 1)) {}

  void SetUp() override {
    author1 = std::make_shared<Author>("auth001", "Ken Follett");
    // Book takes shared_ptr<Author>
    book1 = std::make_unique<Book>("book001", "The Pillars of the Earth", author1, "978-0451166890",
                                   1989);
    user1 = User("user001", "Alice Smith");

    Date loan_date = DateTimeUtils::parseDate("2023-01-10").value();
    Date due_date = DateTimeUtils::addDays(loan_date, 14);
    loan1 = LoanRecord("loan001", book1->getId(), user1.getUserId(), loan_date, due_date);
  }
};

TEST_F(InMemoryPersistenceServiceTest, AuthorOperations) {
  service.saveAuthor(author1);
  auto loaded_author_opt = service.loadAuthor("auth001");
  ASSERT_TRUE(loaded_author_opt.has_value());
  std::shared_ptr<Author> loaded_author = loaded_author_opt.value();
  EXPECT_EQ(loaded_author->getName(), "Ken Follett");
  EXPECT_EQ(loaded_author.get(),
            author1.get());  // Should be the same shared_ptr instance

  auto all_authors = service.loadAllAuthors();
  ASSERT_EQ(all_authors.size(), 1);
  EXPECT_EQ(all_authors[0]->getId(), "auth001");

  service.deleteAuthor("auth001");
  EXPECT_FALSE(service.loadAuthor("auth001").has_value());
  EXPECT_TRUE(service.loadAllAuthors().empty());
}

TEST_F(InMemoryPersistenceServiceTest, LibraryItemOperations) {
  // Save the author first as Book depends on it (though InMemory persistence
  // doesn't enforce this relation)
  service.saveAuthor(author1);
  service.saveLibraryItem(book1.get());  // Pass raw pointer from unique_ptr

  auto loaded_item_opt = service.loadLibraryItem("book001");
  ASSERT_TRUE(loaded_item_opt.has_value());
  std::unique_ptr<ILibraryItem> loaded_item = std::move(loaded_item_opt.value());
  ASSERT_NE(loaded_item, nullptr);
  EXPECT_EQ(loaded_item->getTitle(), "The Pillars of the Earth");
  EXPECT_NE(loaded_item.get(),
            book1.get());  // Should be a clone, different instance

  // Check if it's a Book
  Book* loaded_book = dynamic_cast<Book*>(loaded_item.get());
  ASSERT_NE(loaded_book, nullptr);
  EXPECT_EQ(loaded_book->getIsbn(), "978-0451166890");
  ASSERT_NE(loaded_book->getAuthor(), nullptr);
  EXPECT_EQ(loaded_book->getAuthor()->getId(),
            author1->getId());  // Author is shared

  auto all_items = service.loadAllLibraryItems();
  ASSERT_EQ(all_items.size(), 1);
  EXPECT_EQ(all_items[0]->getId(), "book001");

  service.deleteLibraryItem("book001");
  EXPECT_FALSE(service.loadLibraryItem("book001").has_value());
  EXPECT_TRUE(service.loadAllLibraryItems().empty());
}

TEST_F(InMemoryPersistenceServiceTest, UserOperations) {
  service.saveUser(user1);
  auto loaded_user_opt = service.loadUser("user001");
  ASSERT_TRUE(loaded_user_opt.has_value());
  User loaded_user = loaded_user_opt.value();
  EXPECT_EQ(loaded_user.getName(), "Alice Smith");
  // For value types, it's a copy
  EXPECT_NE(&loaded_user, &user1);

  auto all_users = service.loadAllUsers();
  ASSERT_EQ(all_users.size(), 1);
  EXPECT_EQ(all_users[0].getUserId(), "user001");

  service.deleteUser("user001");
  EXPECT_FALSE(service.loadUser("user001").has_value());
  EXPECT_TRUE(service.loadAllUsers().empty());
}

TEST_F(InMemoryPersistenceServiceTest, LoanRecordOperations) {
  service.saveLoanRecord(loan1);
  auto loaded_loan_opt = service.loadLoanRecord("loan001");
  ASSERT_TRUE(loaded_loan_opt.has_value());
  LoanRecord loaded_loan = loaded_loan_opt.value();
  EXPECT_EQ(loaded_loan.getItemId(), book1->getId());
  EXPECT_EQ(loaded_loan.getUserId(), user1.getUserId());

  auto all_loans = service.loadAllLoanRecords();
  ASSERT_EQ(all_loans.size(), 1);
  EXPECT_EQ(all_loans[0].getRecordId(), "loan001");

  auto user_loans = service.loadLoanRecordsByUserId(user1.getUserId());
  ASSERT_EQ(user_loans.size(), 1);
  EXPECT_EQ(user_loans[0].getRecordId(), "loan001");

  auto item_loans = service.loadLoanRecordsByItemId(book1->getId());
  ASSERT_EQ(item_loans.size(), 1);
  EXPECT_EQ(item_loans[0].getRecordId(), "loan001");

  // Test update
  Date new_due_date = DateTimeUtils::addDays(loan1.getDueDate(), 7);
  LoanRecord updated_loan = loan1;  // Create a copy to modify
  // No, create from existing data for clarity
  updated_loan = LoanRecord(loan1.getRecordId(), loan1.getItemId(), loan1.getUserId(),
                            loan1.getLoanDate(), new_due_date);
  Date return_date = DateTimeUtils::addDays(loan1.getLoanDate(), 5);
  // Create a temporary LoanRecord for the update, or modify a copy of loan1
  LoanRecord record_to_update = loan1;          // Make a copy
  record_to_update.setDueDate(new_due_date);    // Set new due date on the copy
  record_to_update.setReturnDate(return_date);  // Set return date on the copy

  service.updateLoanRecord(record_to_update);  // Pass the modified copy

  auto reloaded_loan_opt = service.loadLoanRecord("loan001");
  ASSERT_TRUE(reloaded_loan_opt.has_value());  // Ensure the record was loaded

  const LoanRecord& reloaded_loan =
      reloaded_loan_opt.value();  // Get a reference to the loaded record

  EXPECT_EQ(reloaded_loan.getDueDate(), new_due_date);

  // --- CORRECTED ASSERTS ---
  ASSERT_TRUE(reloaded_loan.getReturnDate().has_value());  // Check if the optional has a value
  EXPECT_EQ(reloaded_loan.getReturnDate().value(), return_date);  // Compare the contained value

  service.deleteLoanRecord("loan001");
  EXPECT_FALSE(service.loadLoanRecord("loan001").has_value());
  EXPECT_TRUE(service.loadAllLoanRecords().empty());
}

TEST_F(InMemoryPersistenceServiceTest, LoadNonExistent) {
  EXPECT_FALSE(service.loadAuthor("nonexistent").has_value());
  EXPECT_FALSE(service.loadLibraryItem("nonexistent").has_value());
  EXPECT_FALSE(service.loadUser("nonexistent").has_value());
  EXPECT_FALSE(service.loadLoanRecord("nonexistent").has_value());
}