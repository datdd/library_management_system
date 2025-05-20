#ifndef LMS_LOAN_SERVICE_I_LOAN_SERVICE_H
#define LMS_LOAN_SERVICE_I_LOAN_SERVICE_H

#include <optional>
#include <vector>
#include "domain_core/loan_record.h"
#include "domain_core/types.h"  // For EntityId, Date, exceptions

namespace lms {
namespace loan_service {

using namespace domain_core;

class ILoanService {
public:
  virtual ~ILoanService() = default;

  // Borrows an item for a user.
  // Throws NotFoundException if user or item does not exist.
  // Throws OperationFailedException if item is not available or other loan conditions are not met.
  // Returns the created LoanRecord.
  virtual LoanRecord borrowItem(const EntityId& user_id, const EntityId& item_id) = 0;

  // Returns an item borrowed by a user.
  // Throws NotFoundException if the specific loan record (or active loan for user/item) does not
  // exist. Throws OperationFailedException if item cannot be returned (e.g., already returned).
  virtual void returnItem(const EntityId& user_id, const EntityId& item_id) = 0;
  // Alternative: returnItem by loan_record_id might be more precise
  // virtual void returnItemByLoanId(const EntityId& loan_record_id) = 0;

  // Retrieves all active loans for a specific user.
  virtual std::vector<LoanRecord> getActiveLoansForUser(const EntityId& user_id) const = 0;

  // Retrieves the loan history for a specific user (both active and returned).
  virtual std::vector<LoanRecord> getLoanHistoryForUser(const EntityId& user_id) const = 0;

  // Retrieves the loan history for a specific item.
  virtual std::vector<LoanRecord> getLoanHistoryForItem(const EntityId& item_id) const = 0;

  // (Optional) Check for overdue items and send notifications.
  // This might be a separate scheduled task in a real system.
  virtual void processOverdueItems() = 0;
};

}  // namespace loan_service
}  // namespace lms

#endif  // LMS_LOAN_SERVICE_I_LOAN_SERVICE_H