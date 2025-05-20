#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "loan_service/loan_service.h"
#include "mock_catalog_service.h"
#include "mock_notification_service.h"
#include "mock_persistence_service.h"
#include "mock_user_service.h"

#include "domain_core/book.h"
#include "domain_core/types.h"
#include "utils/date_time_utils.h"

using namespace lms::loan_service;
using namespace lms::domain_core;
using namespace lms::utils;
namespace tst = lms::testing;  // Alias for mock namespace

using ::testing::_;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;  // To capture arguments

class LoanServiceTest : public ::testing::Test {
protected:
  std::shared_ptr<NiceMock<tst::MockCatalogService>> mock_catalog_service_;
  std::shared_ptr<NiceMock<tst::MockUserService>> mock_user_service_;
  std::shared_ptr<NiceMock<tst::MockPersistenceService>> mock_persistence_service_;
  std::shared_ptr<NiceMock<tst::MockNotificationService>> mock_notification_service_;
  std::shared_ptr<DateTimeUtils> date_time_utils_;  // Use real DateTimeUtils

  std::unique_ptr<LoanService> loan_service_;

  User test_user_;
  std::shared_ptr<Author> test_author_;

  EntityId available_item_id_ = "item_avail";
  EntityId borrowed_item_id_ = "item_borrowed";
  int default_loan_days_ = 7;

  LoanServiceTest()
      : test_user_("user1", "Test User"),
        test_author_(std::make_shared<Author>("author1", "Test Author")) {}

  void SetUp() override {
    mock_catalog_service_ = std::make_shared<NiceMock<tst::MockCatalogService>>();
    mock_user_service_ = std::make_shared<NiceMock<tst::MockUserService>>();
    mock_persistence_service_ = std::make_shared<NiceMock<tst::MockPersistenceService>>();
    mock_notification_service_ = std::make_shared<NiceMock<tst::MockNotificationService>>();
    date_time_utils_ = std::make_shared<DateTimeUtils>();  // Real instance

    loan_service_ = std::make_unique<LoanService>(
        mock_catalog_service_, mock_user_service_, mock_persistence_service_,
        mock_notification_service_, date_time_utils_, default_loan_days_);
  }

public:
  // Helper to create a Book unique_ptr for test returns by mocks
  std::unique_ptr<ILibraryItem> CreateTestBook(
      const EntityId& id,
      const std::string& title,
      std::shared_ptr<Author> author,
      const std::string& isbn,
      int year,
      AvailabilityStatus status = AvailabilityStatus::AVAILABLE) {
    return std::make_unique<Book>(id, title, std::move(author), isbn, year, status);
  }
};

TEST_F(LoanServiceTest, BorrowItemSuccessfully) {
  EXPECT_CALL(*mock_user_service_, findUserById(test_user_.getUserId()))
      .WillOnce(Return(test_user_));
  EXPECT_CALL(*mock_catalog_service_, findItemById(available_item_id_))
      .WillOnce(Return(ByMove(CreateTestBook(available_item_id_, "Available Book", test_author_,
                                             "isbn1", 2020, AvailabilityStatus::AVAILABLE))));

  EXPECT_CALL(*mock_persistence_service_, loadLoanRecordsByUserId(test_user_.getUserId()))
      .WillOnce(Return(std::vector<LoanRecord>{}));

  // Initialize saved_loan_record with valid dummy values
  LoanRecord saved_loan_record("dummy_id", "dummy_item", "dummy_user", date_time_utils_->now(),
                               date_time_utils_->addDays(date_time_utils_->now(), 1));

  EXPECT_CALL(*mock_persistence_service_, saveLoanRecord(_))
      .WillOnce(SaveArg<0>(&saved_loan_record));  // Capture the saved loan record

  EXPECT_CALL(*mock_catalog_service_,
              updateItemStatus(available_item_id_, AvailabilityStatus::BORROWED))
      .Times(1);

  LoanRecord result_loan = loan_service_->borrowItem(test_user_.getUserId(), available_item_id_);

  EXPECT_EQ(result_loan.getItemId(), available_item_id_);
  EXPECT_EQ(result_loan.getUserId(), test_user_.getUserId());
  EXPECT_FALSE(result_loan.getReturnDate().has_value());

  // Check the captured saved_loan_record (it will be overwritten by SaveArg)
  EXPECT_EQ(saved_loan_record.getItemId(), available_item_id_);
  EXPECT_EQ(saved_loan_record.getUserId(), test_user_.getUserId());

  auto expected_due_date =
      date_time_utils_->addDays(saved_loan_record.getLoanDate(), default_loan_days_);
  // Due to 'now()' potentially differing slightly between object construction and check,
  // compare date parts or allow a small tolerance for time_points if comparing directly.
  // For simplicity, let's assume the loan date from the captured record is the reference.
  EXPECT_EQ(date_time_utils_->formatDate(saved_loan_record.getDueDate()),
            date_time_utils_->formatDate(expected_due_date));
}

TEST_F(LoanServiceTest, BorrowItemFailsUserNotFound) {
  EXPECT_CALL(*mock_user_service_, findUserById("unknown_user")).WillOnce(Return(std::nullopt));
  EXPECT_THROW(loan_service_->borrowItem("unknown_user", available_item_id_), NotFoundException);
}

TEST_F(LoanServiceTest, BorrowItemFailsItemNotFound) {
  EXPECT_CALL(*mock_user_service_, findUserById(test_user_.getUserId()))
      .WillOnce(Return(test_user_));
  EXPECT_CALL(*mock_catalog_service_, findItemById("unknown_item")).WillOnce(Return(std::nullopt));
  EXPECT_THROW(loan_service_->borrowItem(test_user_.getUserId(), "unknown_item"),
               NotFoundException);
}

TEST_F(LoanServiceTest, BorrowItemFailsItemNotAvailable) {
  EXPECT_CALL(*mock_user_service_, findUserById(test_user_.getUserId()))
      .WillOnce(Return(test_user_));
  EXPECT_CALL(*mock_catalog_service_, findItemById(borrowed_item_id_))
      .WillOnce(Return(ByMove(CreateTestBook(borrowed_item_id_, "Borrowed Book", test_author_,
                                             "isbn2", 2021, AvailabilityStatus::BORROWED))));

  EXPECT_THROW(loan_service_->borrowItem(test_user_.getUserId(), borrowed_item_id_),
               OperationFailedException);
}

TEST_F(LoanServiceTest, BorrowItemFailsIfUserAlreadyHasItem) {
  EXPECT_CALL(*mock_user_service_, findUserById(test_user_.getUserId()))
      .WillOnce(Return(test_user_));
  EXPECT_CALL(*mock_catalog_service_, findItemById(available_item_id_))
      .WillOnce(Return(ByMove(CreateTestBook(available_item_id_, "Available Book", test_author_,
                                             "isbn1", 2020, AvailabilityStatus::AVAILABLE))));

  LoanRecord existing_loan("loan_old", available_item_id_, test_user_.getUserId(),
                           date_time_utils_->now(),
                           date_time_utils_->addDays(date_time_utils_->now(), 5));
  std::vector<LoanRecord> user_loans = {existing_loan};
  EXPECT_CALL(*mock_persistence_service_, loadLoanRecordsByUserId(test_user_.getUserId()))
      .WillOnce(Return(user_loans));

  EXPECT_THROW(loan_service_->borrowItem(test_user_.getUserId(), available_item_id_),
               OperationFailedException);
}

TEST_F(LoanServiceTest, ReturnItemSuccessfully) {
  // In TEST_F(LoanServiceTest, ReturnItemSuccessfully)
  Date loan_d = date_time_utils_->addDays(date_time_utils_->now(), -5);
  Date due_d = date_time_utils_->addDays(loan_d, default_loan_days_);
  LoanRecord active_loan("loan789", available_item_id_, test_user_.getUserId(), loan_d, due_d);
  std::vector<LoanRecord> item_loans = {active_loan};

  EXPECT_CALL(*mock_persistence_service_, loadLoanRecordsByItemId(available_item_id_))
      .WillOnce(Return(item_loans));

  LoanRecord updated_loan_record("dummy_update", "dummy_item", "dummy_user",
                                 date_time_utils_->now(),
                                 date_time_utils_->addDays(date_time_utils_->now(), 1));

  EXPECT_CALL(*mock_persistence_service_, updateLoanRecord(_))
      .WillOnce(SaveArg<0>(&updated_loan_record));
  EXPECT_CALL(*mock_catalog_service_,
              updateItemStatus(available_item_id_, AvailabilityStatus::AVAILABLE))
      .Times(1);

  ASSERT_NO_THROW(loan_service_->returnItem(test_user_.getUserId(), available_item_id_));
  
  ASSERT_TRUE(updated_loan_record.getReturnDate().has_value());
  // Check if return date is recent
  auto time_diff = date_time_utils_->now() - updated_loan_record.getReturnDate().value();
  // Allow a small delta for the time difference
  EXPECT_LE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(time_diff).count()), 2);
}

TEST_F(LoanServiceTest, ReturnItemFailsNoActiveLoan) {
  EXPECT_CALL(*mock_persistence_service_, loadLoanRecordsByItemId(available_item_id_))
      .WillOnce(Return(std::vector<LoanRecord>{}));  // No loans for this item

  EXPECT_THROW(loan_service_->returnItem(test_user_.getUserId(), available_item_id_),
               NotFoundException);
}

TEST_F(LoanServiceTest, GetActiveLoansForUser) {
  Date now = date_time_utils_->now();
  LoanRecord active("l1", "i1", test_user_.getUserId(), now, date_time_utils_->addDays(now, 7));
  LoanRecord returned("l2", "i2", test_user_.getUserId(), now, date_time_utils_->addDays(now, 7));
  returned.setReturnDate(now);  // Mark as returned

  std::vector<LoanRecord> user_history = {active, returned};
  EXPECT_CALL(*mock_persistence_service_, loadLoanRecordsByUserId(test_user_.getUserId()))
      .WillOnce(Return(user_history));

  auto active_loans = loan_service_->getActiveLoansForUser(test_user_.getUserId());
  ASSERT_EQ(active_loans.size(), 1);
  EXPECT_EQ(active_loans[0].getRecordId(), "l1");
}

TEST_F(LoanServiceTest, ProcessOverdueItemsSendsNotification) {
  Date today = date_time_utils_->today();
  Date overdue_due_date = date_time_utils_->addDays(today, -1);  // Due yesterday
  Date future_due_date = date_time_utils_->addDays(today, 1);    // Due tomorrow

  LoanRecord overdue_loan("overdue1", "item_over", "user_over",
                          date_time_utils_->addDays(today, -10), overdue_due_date);
  LoanRecord active_not_overdue("active1", "item_active", "user_norm",
                                date_time_utils_->addDays(today, -5), future_due_date);
  LoanRecord returned_loan("returned1", "item_ret", "user_ret",
                           date_time_utils_->addDays(today, -15),
                           date_time_utils_->addDays(today, -1));
  returned_loan.setReturnDate(date_time_utils_->addDays(today, -2));  // Returned before today

  std::vector<LoanRecord> all_loans = {overdue_loan, active_not_overdue, returned_loan};
  EXPECT_CALL(*mock_persistence_service_, loadAllLoanRecords()).WillOnce(Return(all_loans));

  // Mock returns for user and item details for the overdue item
  EXPECT_CALL(*mock_user_service_, findUserById("user_over"))
      .WillRepeatedly(Return(User("user_over", "Overdue User")));
  EXPECT_CALL(*mock_catalog_service_, findItemById("item_over"))
      .WillRepeatedly(Return(
          ByMove(CreateTestBook("item_over", "Overdue Book", test_author_, "isbn_over", 2000))));

  // Only one notification should be sent for the overdue_loan
  EXPECT_CALL(*mock_notification_service_,
              sendNotification("user_over", ::testing::HasSubstr("was due on")))
      .Times(1);
  // Ensure no notification for other users
  EXPECT_CALL(*mock_notification_service_, sendNotification("user_norm", _)).Times(0);
  EXPECT_CALL(*mock_notification_service_, sendNotification("user_ret", _)).Times(0);

  loan_service_->processOverdueItems();
}