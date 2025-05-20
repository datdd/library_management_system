#include "domain_core/loan_record.h"
#include <chrono>
#include "domain_core/types.h"
#include "gtest/gtest.h"

using namespace lms::domain_core;
using namespace std::chrono_literals;  // For s, h, etc.

TEST(LoanRecordTest, ConstructorAndGetters) {
  Date loan_date = std::chrono::system_clock::now();
  Date due_date = loan_date + 24h * 14;  // 14 days

  LoanRecord record("lr1", "item1", "user1", loan_date, due_date);
  EXPECT_EQ(record.getRecordId(), "lr1");
  EXPECT_EQ(record.getItemId(), "item1");
  EXPECT_EQ(record.getUserId(), "user1");
  EXPECT_EQ(record.getLoanDate(), loan_date);
  EXPECT_EQ(record.getDueDate(), due_date);
  ASSERT_FALSE(record.getReturnDate().has_value());
}

TEST(LoanRecordTest, SetDueDate) {
  Date loan_date = std::chrono::system_clock::now();
  Date initial_due_date = loan_date + 24h * 7;
  LoanRecord record("lr2", "item2", "user2", loan_date, initial_due_date);

  Date new_due_date = loan_date + 24h * 10;
  record.setDueDate(new_due_date);
  EXPECT_EQ(record.getDueDate(), new_due_date);
}

TEST(LoanRecordTest, SetReturnDate) {
  Date loan_date = std::chrono::system_clock::now();
  Date due_date = loan_date + 24h * 14;
  LoanRecord record("lr3", "item3", "user3", loan_date, due_date);

  Date return_d = loan_date + 24h * 5;
  record.setReturnDate(return_d);
  ASSERT_TRUE(record.getReturnDate().has_value());
  EXPECT_EQ(record.getReturnDate().value(), return_d);
}

TEST(LoanRecordTest, ConstructorValidations) {
  Date now = std::chrono::system_clock::now();
  EXPECT_THROW(LoanRecord("", "i1", "u1", now, now + 1h), InvalidArgumentException);
  EXPECT_THROW(LoanRecord("lr1", "", "u1", now, now + 1h), InvalidArgumentException);
  EXPECT_THROW(LoanRecord("lr1", "i1", "", now, now + 1h), InvalidArgumentException);
  EXPECT_THROW(LoanRecord("lr1", "i1", "u1", now, now - 1h), InvalidArgumentException);
}

TEST(LoanRecordTest, SetDueDateValidation) {
  Date now = std::chrono::system_clock::now();
  LoanRecord record("lr4", "item4", "user4", now, now + 24h);
  EXPECT_THROW(record.setDueDate(now - 1h), InvalidArgumentException);
}

TEST(LoanRecordTest, SetReturnDateValidation) {
  Date now = std::chrono::system_clock::now();
  LoanRecord record("lr5", "item5", "user5", now, now + 24h);
  EXPECT_THROW(record.setReturnDate(now - 1h), InvalidArgumentException);
}