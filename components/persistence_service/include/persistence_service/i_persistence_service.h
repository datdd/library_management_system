#ifndef LMS_PERSISTENCE_SERVICE_I_PERSISTENCE_SERVICE_H
#define LMS_PERSISTENCE_SERVICE_I_PERSISTENCE_SERVICE_H

#include "domain_core/author.h"
#include "domain_core/i_library_item.h"
#include "domain_core/loan_record.h"
#include "domain_core/types.h"
#include "domain_core/user.h"

#include <memory>  // For unique_ptr, shared_ptr
#include <optional>
#include <vector>

namespace lms {
namespace persistence_service {

using namespace domain_core;  // Make domain types easily accessible

class IPersistenceService {
public:
  virtual ~IPersistenceService() = default;

  // Author Operations
  virtual void saveAuthor(const std::shared_ptr<Author>& author) = 0;
  virtual std::optional<std::shared_ptr<Author>> loadAuthor(const EntityId& author_id) = 0;
  virtual std::vector<std::shared_ptr<Author>> loadAllAuthors() = 0;
  virtual void deleteAuthor(const EntityId& author_id) = 0;  // Added for completeness

  // Library Item Operations
  // The persistence layer will need to clone items to store them,
  // and reconstruct them (potentially with a factory) upon loading.
  virtual void saveLibraryItem(const ILibraryItem* item) = 0;  // Pass by pointer to const
  // Load method returns a new unique_ptr, transferring ownership to caller
  virtual std::optional<std::unique_ptr<ILibraryItem>> loadLibraryItem(const EntityId& item_id) = 0;
  virtual std::vector<std::unique_ptr<ILibraryItem>> loadAllLibraryItems() = 0;
  virtual void deleteLibraryItem(const EntityId& item_id) = 0;

  // User Operations
  virtual void saveUser(const User& user) = 0;
  virtual std::optional<User> loadUser(const EntityId& user_id) = 0;
  virtual std::vector<User> loadAllUsers() = 0;
  virtual void deleteUser(const EntityId& user_id) = 0;  // Added

  // Loan Record Operations
  virtual void saveLoanRecord(const LoanRecord& record) = 0;
  virtual std::optional<LoanRecord> loadLoanRecord(const EntityId& record_id) = 0;
  virtual std::vector<LoanRecord> loadLoanRecordsByUserId(const EntityId& user_id) = 0;
  virtual std::vector<LoanRecord> loadLoanRecordsByItemId(const EntityId& item_id) = 0;
  virtual std::vector<LoanRecord> loadAllLoanRecords() = 0;
  virtual void deleteLoanRecord(const EntityId& record_id) = 0;  // Added
  // It might also be useful to update a loan record (e.g. when item is returned)
  virtual void updateLoanRecord(const LoanRecord& record) = 0;
};

}  // namespace persistence_service
}  // namespace lms

#endif  // LMS_PERSISTENCE_SERVICE_I_PERSISTENCE_SERVICE_H