#include "loan_service/loan_service.h"
#include "domain_core/i_library_item.h"  // For AvailabilityStatus
#include "domain_core/types.h"           // For exceptions, EntityId, Date

#include <algorithm>  // For std::find_if
#include <stdexcept>  // For std::invalid_argument

namespace lms {
namespace loan_service {

LoanService::LoanService(
    std::shared_ptr<catalog_service::ICatalogService> catalog_service,
    std::shared_ptr<user_service::IUserService> user_service,
    std::shared_ptr<persistence_service::IPersistenceService> persistence_service,
    std::shared_ptr<notification_service::INotificationService> notification_service,
    std::shared_ptr<utils::DateTimeUtils> date_time_utils,
    int default_loan_duration_days)
    : m_catalog_service(std::move(catalog_service)),
      m_user_service(std::move(user_service)),
      m_persistence_service(std::move(persistence_service)),
      m_notification_service(std::move(notification_service)),
      m_date_time_utils(std::move(date_time_utils)),
      m_default_loan_duration_days(default_loan_duration_days) {
  if (!m_catalog_service)
    throw std::invalid_argument("Catalog service cannot be null.");
  if (!m_user_service)
    throw std::invalid_argument("User service cannot be null.");
  if (!m_persistence_service)
    throw std::invalid_argument("Persistence service cannot be null.");
  if (!m_notification_service)
    throw std::invalid_argument("Notification service cannot be null.");
  if (!m_date_time_utils)
    throw std::invalid_argument("DateTime utils cannot be null.");
  if (m_default_loan_duration_days <= 0)
    throw std::invalid_argument("Default loan duration must be positive.");
}

EntityId LoanService::generateLoanRecordId() const {
  std::lock_guard<std::mutex> lock(m_id_mutex);
  return "loan_" + std::to_string(++m_next_loan_id_counter);
}

LoanRecord LoanService::borrowItem(const EntityId& user_id, const EntityId& item_id) {
  if (user_id.empty() || item_id.empty()) {
    throw InvalidArgumentException("User ID and Item ID cannot be empty for borrowing.");
  }

  // 1. Validate user
  auto user_opt = m_user_service->findUserById(user_id);
  if (!user_opt.has_value()) {
    throw NotFoundException("User with ID '" + user_id + "' not found.");
  }

  // 2. Validate item and its availability
  auto item_opt = m_catalog_service->findItemById(item_id);
  if (!item_opt.has_value() || !item_opt.value()) {
    throw NotFoundException("Library item with ID '" + item_id + "' not found.");
  }
  std::unique_ptr<ILibraryItem> item = std::move(item_opt.value());  // Take ownership

  if (item->getAvailabilityStatus() != AvailabilityStatus::AVAILABLE) {
    throw OperationFailedException("Item '" + item_id +
                                   "' is not available for borrowing. Status: " +
                                   std::to_string(static_cast<int>(item->getAvailabilityStatus())));
  }

  // (Optional: Check if user already has this item borrowed and not returned)
  auto user_loans = getActiveLoansForUser(user_id);
  auto it_existing_loan =
      std::find_if(user_loans.begin(), user_loans.end(),
                   [&](const LoanRecord& lr) { return lr.getItemId() == item_id; });
  if (it_existing_loan != user_loans.end()) {
    throw OperationFailedException("User '" + user_id + "' has already borrowed item '" + item_id +
                                   "'.");
  }

  // 3. Create Loan Record
  Date loan_date = m_date_time_utils->now();
  Date due_date = m_date_time_utils->addDays(loan_date, m_default_loan_duration_days);
  EntityId loan_id = generateLoanRecordId();
  LoanRecord new_loan(loan_id, item_id, user_id, loan_date, due_date);

  // 4. Persist Loan Record
  m_persistence_service->saveLoanRecord(new_loan);

  // 5. Update Item Status in Catalog
  m_catalog_service->updateItemStatus(item_id, AvailabilityStatus::BORROWED);

  // (Optional: Send notification)
  // m_notification_service->sendNotification(user_id, "Item '" + item->getTitle() + "' borrowed
  // successfully. Due: " + m_date_time_utils->formatDate(due_date));

  return new_loan;
}

void LoanService::returnItem(const EntityId& user_id, const EntityId& item_id) {
  if (user_id.empty() || item_id.empty()) {
    throw InvalidArgumentException("User ID and Item ID cannot be empty for returning.");
  }

  // 1. Find the active loan record for this user and item.
  //    An item can only be loaned once at a time by a specific user.
  std::vector<LoanRecord> all_item_loans = m_persistence_service->loadLoanRecordsByItemId(item_id);
  LoanRecord* active_loan_ptr = nullptr;
  EntityId active_loan_id_to_update;

  for (auto& loan : all_item_loans) {  // Iterate by ref to modify if needed
    if (loan.getUserId() == user_id && !loan.getReturnDate().has_value()) {
      active_loan_ptr = &loan;  // Found the active loan
      active_loan_id_to_update = loan.getRecordId();
      break;
    }
  }

  if (!active_loan_ptr) {
    throw NotFoundException("No active loan found for user '" + user_id + "' and item '" + item_id +
                            "'.");
  }

  // 2. Update Loan Record with return date
  active_loan_ptr->setReturnDate(m_date_time_utils->now());
  m_persistence_service->updateLoanRecord(*active_loan_ptr);

  // 3. Update Item Status in Catalog
  m_catalog_service->updateItemStatus(item_id, AvailabilityStatus::AVAILABLE);

  // (Optional: Check for fines if overdue and send notification)
  // if (active_loan_ptr->getReturnDate().value() > active_loan_ptr->getDueDate()) {
  //     m_notification_service->sendNotification(user_id, "Item '" + item_id + "' returned
  //     overdue.");
  // } else {
  //     m_notification_service->sendNotification(user_id, "Item '" + item_id + "' returned
  //     successfully.");
  // }
}

std::vector<LoanRecord> LoanService::getActiveLoansForUser(const EntityId& user_id) const {
  if (user_id.empty()) {
    throw InvalidArgumentException("User ID cannot be empty.");
  }
  std::vector<LoanRecord> user_history = m_persistence_service->loadLoanRecordsByUserId(user_id);
  std::vector<LoanRecord> active_loans;
  for (const auto& record : user_history) {
    if (!record.getReturnDate().has_value()) {  // No return date means active
      active_loans.push_back(record);
    }
  }
  return active_loans;
}

std::vector<LoanRecord> LoanService::getLoanHistoryForUser(const EntityId& user_id) const {
  if (user_id.empty()) {
    throw InvalidArgumentException("User ID cannot be empty.");
  }
  return m_persistence_service->loadLoanRecordsByUserId(user_id);
}

std::vector<LoanRecord> LoanService::getLoanHistoryForItem(const EntityId& item_id) const {
  if (item_id.empty()) {
    throw InvalidArgumentException("Item ID cannot be empty.");
  }
  return m_persistence_service->loadLoanRecordsByItemId(item_id);
}

void LoanService::processOverdueItems() {
  std::vector<LoanRecord> all_loans = m_persistence_service->loadAllLoanRecords();
  Date today = m_date_time_utils->today();  // Get midnight today for comparison

  for (const auto& loan : all_loans) {
    if (!loan.getReturnDate().has_value() && loan.getDueDate() < today) {
      // This item is overdue
      std::optional<User> user_opt = m_user_service->findUserById(loan.getUserId());
      std::optional<std::unique_ptr<ILibraryItem>> item_opt =
          m_catalog_service->findItemById(loan.getItemId());

      std::string user_name = user_opt.has_value() ? user_opt.value().getName() : "Unknown User";
      std::string item_title = (item_opt.has_value() && item_opt.value())
                                   ? item_opt.value()->getTitle()
                                   : "Unknown Item";

      std::string message = "Dear " + user_name + ", the item '" + item_title +
                            "' (Loan ID: " + loan.getRecordId() + ") was due on " +
                            m_date_time_utils->formatDate(loan.getDueDate()) +
                            ". Please return it as soon as possible.";
      m_notification_service->sendNotification(loan.getUserId(), message);
    }
  }
}

}  // namespace loan_service
}  // namespace lms