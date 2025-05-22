#ifndef LMS_PERSISTENCE_SERVICE_CACHING_FILE_PERSISTENCE_SERVICE_H
#define LMS_PERSISTENCE_SERVICE_CACHING_FILE_PERSISTENCE_SERVICE_H

#include <memory>
#include <string>
#include "file_persistence_service.h"  // To hold one internally
#include "i_persistence_service.h"
#include "in_memory_persistence_service.h"  // To hold one internally

namespace lms {
namespace persistence_service {

class CachingFilePersistenceService : public IPersistenceService {
public:
  CachingFilePersistenceService(const std::string& data_directory_path,
                                std::shared_ptr<utils::DateTimeUtils> date_time_utils);

  // Standard IPersistenceService methods - mostly delegate to in-memory store
  void saveAuthor(const std::shared_ptr<Author>& author) override;
  std::optional<std::shared_ptr<Author>> loadAuthor(const EntityId& author_id) override;
  std::vector<std::shared_ptr<Author>> loadAllAuthors() override;
  void deleteAuthor(const EntityId& author_id) override;

  void saveLibraryItem(const ILibraryItem* item) override;
  std::optional<std::unique_ptr<ILibraryItem>> loadLibraryItem(const EntityId& item_id) override;
  std::vector<std::unique_ptr<ILibraryItem>> loadAllLibraryItems() override;
  void deleteLibraryItem(const EntityId& item_id) override;

  void saveUser(const User& user) override;
  std::optional<User> loadUser(const EntityId& user_id) override;
  std::vector<User> loadAllUsers() override;
  void deleteUser(const EntityId& user_id) override;

  void saveLoanRecord(const LoanRecord& record) override;
  std::optional<LoanRecord> loadLoanRecord(const EntityId& record_id) override;
  std::vector<LoanRecord> loadLoanRecordsByUserId(const EntityId& user_id) override;
  std::vector<LoanRecord> loadLoanRecordsByItemId(const EntityId& item_id) override;
  std::vector<LoanRecord> loadAllLoanRecords() override;
  void deleteLoanRecord(const EntityId& record_id) override;
  void updateLoanRecord(const LoanRecord& record) override;

  // Special method to trigger saving all in-memory data to files
  void persistAllToFile();
  // Special method to load all from file to memory (called at construction)
  void loadAllFromFileToMemory();

private:
  std::unique_ptr<InMemoryPersistenceService> m_memory_store;
  std::unique_ptr<FilePersistenceService> m_file_store;
  // No DateTimeUtils needed directly here if FilePersistenceService handles it.
};

}  // namespace persistence_service
}  // namespace lms

#endif  // LMS_PERSISTENCE_SERVICE_CACHING_FILE_PERSISTENCE_SERVICE_H