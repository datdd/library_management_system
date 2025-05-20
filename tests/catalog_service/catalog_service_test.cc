#include "catalog_service/catalog_service.h"  // Class to test
#include "domain_core/book.h"
#include "domain_core/types.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mock_persistence_service.h"

using namespace lms::catalog_service;
using namespace lms::domain_core;
using namespace lms::testing;  // For MockPersistenceService

using ::testing::_;
using ::testing::ByMove;  // For returning unique_ptr
using ::testing::NiceMock;
using ::testing::Pointee;  // To match content of a pointer
using ::testing::Return;
using ::testing::Truly;

// Helper to create a Book unique_ptr for test returns
std::unique_ptr<ILibraryItem> CreateTestBook(
    const EntityId& id,
    const std::string& title,
    std::shared_ptr<Author> author,
    const std::string& isbn,
    int year,
    AvailabilityStatus status = AvailabilityStatus::AVAILABLE) {
  return std::make_unique<Book>(id, title, std::move(author), isbn, year, status);
}

class CatalogServiceTest : public ::testing::Test {
protected:
  std::shared_ptr<NiceMock<MockPersistenceService>> mock_persistence_service_;
  std::unique_ptr<CatalogService> catalog_service_;

  std::shared_ptr<Author> test_author1;
  std::shared_ptr<Author> test_author2;

  CatalogServiceTest() {}

  void SetUp() override {
    mock_persistence_service_ = std::make_shared<NiceMock<MockPersistenceService>>();
    catalog_service_ = std::make_unique<CatalogService>(mock_persistence_service_);

    test_author1 = std::make_shared<Author>("auth1", "Author One");
    test_author2 = std::make_shared<Author>("auth2", "Author Two");
  }
};

TEST_F(CatalogServiceTest, AddBookSuccessfullyNewAuthor) {
  EntityId item_id = "book123";
  std::string title = "The Great Book";
  EntityId author_id = "new_auth";
  std::string author_name = "New Author";
  std::string isbn = "12345";
  int year = 2023;

  // Expect author load to find nothing
  EXPECT_CALL(*mock_persistence_service_, loadAuthor(author_id)).WillOnce(Return(std::nullopt));
  // Expect new author to be saved
  EXPECT_CALL(*mock_persistence_service_,
              saveAuthor(Truly([&](const std::shared_ptr<Author>& auth) {
                return auth && auth->getId() == author_id && auth->getName() == author_name;
              })))
      .Times(1);

  // Expect item load to find nothing
  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem(item_id)).WillOnce(Return(std::nullopt));
  // Expect item to be saved
  EXPECT_CALL(*mock_persistence_service_,
              saveLibraryItem(Pointee(Truly([&](const ILibraryItem& item) {
                const auto* book = dynamic_cast<const Book*>(&item);
                return book && book->getId() == item_id && book->getTitle() == title &&
                       book->getAuthor() && book->getAuthor()->getId() == author_id &&
                       book->getIsbn() == isbn && book->getPublicationYear() == year;
              }))))
      .Times(1);

  ASSERT_NO_THROW(catalog_service_->addBook(item_id, title, author_id, author_name, isbn, year));
}

TEST_F(CatalogServiceTest, AddBookSuccessfullyExistingAuthor) {
  EXPECT_CALL(*mock_persistence_service_, loadAuthor("auth1"))
      .WillOnce(Return(test_author1));                              // Author exists
  EXPECT_CALL(*mock_persistence_service_, saveAuthor(_)).Times(0);  // No new author save

  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem("book789"))
      .WillOnce(Return(std::nullopt));  // Item doesn't exist
  EXPECT_CALL(*mock_persistence_service_,
              saveLibraryItem(Pointee(Truly([&](const ILibraryItem& item) {
                return item.getId() == "book789" && item.getAuthor() &&
                       item.getAuthor()->getId() == "auth1";
              }))))
      .Times(1);

  ASSERT_NO_THROW(
      catalog_service_->addBook("book789", "Another Book", "auth1", "Author One", "67890", 2022));
}

TEST_F(CatalogServiceTest, AddBookFailsIfItemExists) {
  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem("book123"))
      .WillOnce(Return(
          CreateTestBook("book123", "Old Title", test_author1, "isbn", 2000)));  // Item exists

  EXPECT_THROW(
      catalog_service_->addBook("book123", "New Title", "auth1", "Author One", "123", 2021),
      OperationFailedException);
}

TEST_F(CatalogServiceTest, FindItemByIdSuccessfully) {
  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem("book1"))
      .WillOnce(
          Return(ByMove(CreateTestBook("book1", "Test Book 1", test_author1, "isbn1", 2020))));

  auto item_opt = catalog_service_->findItemById("book1");
  ASSERT_TRUE(item_opt.has_value());
  ASSERT_NE(item_opt.value(), nullptr);
  EXPECT_EQ(item_opt.value()->getId(), "book1");
}

TEST_F(CatalogServiceTest, GetAllItems) {
  std::vector<std::unique_ptr<ILibraryItem>> mock_items;
  mock_items.push_back(CreateTestBook("b1", "T1", test_author1, "i1", 2001));
  mock_items.push_back(CreateTestBook("b2", "T2", test_author2, "i2", 2002));

  EXPECT_CALL(*mock_persistence_service_, loadAllLibraryItems())
      .WillOnce(Return(ByMove(std::move(mock_items))));  // Must move the vector of unique_ptrs

  auto all_items = catalog_service_->getAllItems();
  ASSERT_EQ(all_items.size(), 2);
  // Due to move, original mock_items is now likely empty or full of nullptrs.
}

TEST_F(CatalogServiceTest, RemoveItemSuccessfully) {
  // Need loadLibraryItem to confirm existence before delete
  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem("bookToDelete"))
      .WillOnce(
          Return(ByMove(CreateTestBook("bookToDelete", "Title", test_author1, "isbn", 2000))));
  EXPECT_CALL(*mock_persistence_service_, deleteLibraryItem("bookToDelete")).Times(1);

  EXPECT_TRUE(catalog_service_->removeItem("bookToDelete"));
}

TEST_F(CatalogServiceTest, RemoveItemReturnsFalseIfNotFound) {
  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem("nonexistent"))
      .WillOnce(Return(std::nullopt));
  EXPECT_CALL(*mock_persistence_service_, deleteLibraryItem("nonexistent")).Times(0);

  EXPECT_FALSE(catalog_service_->removeItem("nonexistent"));
}

TEST_F(CatalogServiceTest, UpdateItemStatusSuccessfully) {
  EntityId item_id = "bookStatus";
  // Initial item state
  auto initial_item = CreateTestBook(item_id, "Status Book", test_author1, "isbnS", 2023,
                                     AvailabilityStatus::AVAILABLE);
  // We need to be careful with unique_ptr ownership for the mock return and subsequent operations.
  // loadLibraryItem returns a new unique_ptr.
  // saveLibraryItem takes a pointer to an item whose status is updated.

  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem(item_id))
      .WillOnce(Return(ByMove(CreateTestBook(item_id, "Status Book", test_author1, "isbnS", 2023,
                                             AvailabilityStatus::AVAILABLE))));

  EXPECT_CALL(*mock_persistence_service_,
              saveLibraryItem(Pointee(Truly([&](const ILibraryItem& item) {
                return item.getId() == item_id &&
                       item.getAvailabilityStatus() == AvailabilityStatus::BORROWED;
              }))))
      .Times(1);

  ASSERT_NO_THROW(catalog_service_->updateItemStatus(item_id, AvailabilityStatus::BORROWED));
}

TEST_F(CatalogServiceTest, UpdateItemStatusThrowsIfItemNotFound) {
  EXPECT_CALL(*mock_persistence_service_, loadLibraryItem("nonexistent"))
      .WillOnce(Return(std::nullopt));
  EXPECT_THROW(catalog_service_->updateItemStatus("nonexistent", AvailabilityStatus::BORROWED),
               NotFoundException);
}