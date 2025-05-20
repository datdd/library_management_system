#ifndef LMS_PERSISTENCE_SERVICE_IN_MEMORY_PERSISTENCE_SERVICE_H
#define LMS_PERSISTENCE_SERVICE_IN_MEMORY_PERSISTENCE_SERVICE_H

#include <algorithm>  // For std::remove_if, std::find_if
#include <map>
#include <memory>
#include <mutex>  // For basic thread safety
#include <optional>
#include <vector>
#include "i_persistence_service.h"

namespace lms {
namespace persistence_service {

class InMemoryPersistenceService : public IPersistenceService {
public:
  InMemoryPersistenceService();
  ~InMemoryPersistenceService() override = default;

  // Author Operations
  void saveAuthor(const std::shared_ptr<Author>& author) override;
  std::optional<std::shared_ptr<Author>> loadAuthor(const EntityId& author_id) override;
  std::vector<std::shared_ptr<Author>> loadAllAuthors() override;
  void deleteAuthor(const EntityId& author_id) override;

  // Library Item Operations
  void saveLibraryItem(const ILibraryItem* item) override;
  std::optional<std::unique_ptr<ILibraryItem>> loadLibraryItem(const EntityId& item_id) override;
  std::vector<std::unique_ptr<ILibraryItem>> loadAllLibraryItems() override;
  void deleteLibraryItem(const EntityId& item_id) override;

  // User Operations
  void saveUser(const User& user) override;
  std::optional<User> loadUser(const EntityId& user_id) override;
  std::vector<User> loadAllUsers() override;
  void deleteUser(const EntityId& user_id) override;

  // Loan Record Operations
  void saveLoanRecord(const LoanRecord& record) override;
  std::optional<LoanRecord> loadLoanRecord(const EntityId& record_id) override;
  std::vector<LoanRecord> loadLoanRecordsByUserId(const EntityId& user_id) override;
  std::vector<LoanRecord> loadLoanRecordsByItemId(const EntityId& item_id) override;
  std::vector<LoanRecord> loadAllLoanRecords() override;
  void deleteLoanRecord(const EntityId& record_id) override;
  void updateLoanRecord(const LoanRecord& record) override;

private:
  // Using std::map for quick lookups by ID.
  // The stored unique_ptr<ILibraryItem> in m_items owns the cloned item.
  std::map<EntityId, std::shared_ptr<Author>> m_authors;
  std::map<EntityId, std::unique_ptr<ILibraryItem>> m_items;
  std::map<EntityId, User> m_users;
  std::map<EntityId, LoanRecord> m_loan_records;

  // Mutex for thread-safe access to the in-memory stores
  // This is a simple approach; more granular locking could be used if contention becomes an issue.
  mutable std::mutex m_mutex;
};

}  // namespace persistence_service
}  // namespace lms

#endif  // LMS_PERSISTENCE_SERVICE_IN_MEMORY_PERSISTENCE_SERVICE_H