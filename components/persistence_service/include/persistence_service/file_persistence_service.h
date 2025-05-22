#ifndef LMS_PERSISTENCE_SERVICE_FILE_PERSISTENCE_SERVICE_H
#define LMS_PERSISTENCE_SERVICE_FILE_PERSISTENCE_SERVICE_H

#include <fstream>  // For file operations
#include <memory>
#include <mutex>  // For thread safety on file access if needed
#include <string>
#include <vector>
#include "i_persistence_service.h"
#include "utils/date_time_utils.h"  // For date formatting/parsing

namespace lms {
namespace persistence_service {

class FilePersistenceService : public IPersistenceService {
public:
  // Constructor takes the base path for data files and DateTimeUtils
  explicit FilePersistenceService(const std::string& data_directory_path,
                                  std::shared_ptr<utils::DateTimeUtils> date_time_utils);
  ~FilePersistenceService() override = default;

  // Disable copy and assign
  FilePersistenceService(const FilePersistenceService&) = delete;
  FilePersistenceService& operator=(const FilePersistenceService&) = delete;

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
  std::string m_data_dir;
  std::shared_ptr<utils::DateTimeUtils> m_date_time_utils;

  // File names
  const std::string m_authors_file = "authors.csv";
  const std::string m_users_file = "users.csv";
  const std::string m_items_file = "items.csv";  // Will store Books, need type if more items
  const std::string m_loans_file = "loans.csv";

  // Helper: Read all lines from a file
  std::vector<std::vector<std::string>> readCsvFile(const std::string& filename) const;
  // Helper: Write all lines to a file (overwrites)
  void writeCsvFile(const std::string& filename, const std::vector<std::vector<std::string>>& data);
  // Helper: Generic save single record (appends or updates for simplicity here we'll use
  // read-modify-write for updates/deletes)
  void appendToCsvFile(const std::string& filename, const std::vector<std::string>& record_fields);

  // Helper for replacing commas in strings to avoid CSV issues (simple version)
  std::string escapeCsvField(const std::string& field) const;
  std::string unescapeCsvField(const std::string& field) const;

  // Mutex for file operations if multiple threads might call service methods
  // For simplicity, assuming single-threaded access from app, or higher-level services handle
  // concurrency. If needed: mutable std::mutex m_file_mutex;
};

}  // namespace persistence_service
}  // namespace lms

#endif  // LMS_PERSISTENCE_SERVICE_FILE_PERSISTENCE_SERVICE_H