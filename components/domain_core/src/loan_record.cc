#include "domain_core/loan_record.h"
#include <optional>             // Ensure this is included for std::optional
#include "domain_core/types.h"  // For InvalidArgumentException

namespace lms {
namespace domain_core {

LoanRecord::LoanRecord(EntityId record_id,
                       EntityId item_id,
                       EntityId user_id,
                       Date loan_date,
                       Date due_date)
    : m_record_id(std::move(record_id)),
      m_item_id(std::move(item_id)),
      m_user_id(std::move(user_id)),
      m_loan_date(loan_date),
      m_due_date(due_date),
      m_return_date(std::nullopt) {
  if (m_record_id.empty()) {
    throw InvalidArgumentException("LoanRecord ID cannot be empty.");
  }
  if (m_item_id.empty()) {
    throw InvalidArgumentException("LoanRecord Item ID cannot be empty.");
  }
  if (m_user_id.empty()) {
    throw InvalidArgumentException("LoanRecord User ID cannot be empty.");
  }
  if (due_date < loan_date) {
    throw InvalidArgumentException("Due date cannot be before loan date.");
  }
}

const EntityId& LoanRecord::getRecordId() const {
  return m_record_id;
}

const EntityId& LoanRecord::getItemId() const {
  return m_item_id;
}

const EntityId& LoanRecord::getUserId() const {
  return m_user_id;
}

const Date& LoanRecord::getLoanDate() const {
  return m_loan_date;
}

const Date& LoanRecord::getDueDate() const {
  return m_due_date;
}

void LoanRecord::setDueDate(Date due_date) {
  if (due_date < m_loan_date) {
    throw InvalidArgumentException("Due date cannot be before loan date.");
  }
  m_due_date = due_date;
}

const std::optional<Date>& LoanRecord::getReturnDate() const {
  return m_return_date;
}

void LoanRecord::setReturnDate(Date return_date) {
  if (return_date < m_loan_date) {
    throw InvalidArgumentException("Return date cannot be before loan date.");
  }
  m_return_date = return_date;
}

// In library_management_system/components/domain_core/include/domain_core/loan_record.h
// ... inside class LoanRecord ...
bool LoanRecord::operator==(const LoanRecord& other) const {
  if (m_record_id != other.m_record_id)
    return false;
  if (m_item_id != other.m_item_id)
    return false;
  if (m_user_id != other.m_user_id)
    return false;
  if (m_loan_date != other.m_loan_date)
    return false;
  if (m_due_date != other.m_due_date)
    return false;

  // Explicitly compare std::optional<Date> for m_return_date
  if (m_return_date.has_value() != other.m_return_date.has_value()) {
    return false;  // Different optional states (one has value, other doesn't)
  }
  if (m_return_date.has_value()) {  // Both have values, so compare the contained Date objects
    if (m_return_date.value() != other.m_return_date.value()) {
      return false;
    }
  }
  // If both are std::nullopt, or both have values and those values were equal, they are considered
  // equal.
  return true;
}

bool LoanRecord::operator!=(const LoanRecord& other) const {
  return !(*this == other);
}

}  // namespace domain_core
}  // namespace lms