// library_management_system/components/persistence_service/include/persistence_service/ms_sql_persistence_service.h
#ifndef LMS_PERSISTENCE_SERVICE_MS_SQL_PERSISTENCE_SERVICE_H
#define LMS_PERSISTENCE_SERVICE_MS_SQL_PERSISTENCE_SERVICE_H

#include "i_persistence_service.h"
#include "utils/date_time_utils.h"  // For date conversion if needed
// #include "simple_odbc_wrapper.h" // Your chosen SQL connector/wrapper
#include <memory>
#include <mutex>  // For connection pool or serializing access if needed
#include <string>
#include <iostream>

// Forward declare your ODBC wrapper if it's complex
namespace odbc_wrapper {
class Connection;
class PreparedStatement; /* ... */
}  // namespace odbc_wrapper

namespace lms {
namespace persistence_service {

class MsSqlPersistenceService : public IPersistenceService {
public:
  // Constructor takes connection string and DateTimeUtils
  explicit MsSqlPersistenceService(const std::string& connection_string,
                                   std::shared_ptr<utils::DateTimeUtils> date_time_utils);
  ~MsSqlPersistenceService() override;  // Ensure connection is closed

  // Disable copy and assign
  MsSqlPersistenceService(const MsSqlPersistenceService&) = delete;
  MsSqlPersistenceService& operator=(const MsSqlPersistenceService&) = delete;

  // --- IPersistenceService methods ---
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

private:
  // std::unique_ptr<SimpleOdbcWrapper> m_db_conn; // Or your chosen connector type
  std::unique_ptr<odbc_wrapper::Connection> m_db_connection;  // Example
  std::string m_connection_string;
  std::shared_ptr<utils::DateTimeUtils> m_date_time_utils;
  mutable std::mutex m_db_mutex;  // To protect shared connection or for critical sections

  // Helper to ensure connection is active (can be more complex with pooling)
  odbc_wrapper::Connection& getConnection();

  // Helper to convert std::chrono::time_point to SQL DATETIME2 string or ODBC timestamp struct
  std::string toSqlDateTimeString(const Date& date) const;
  Date fromSqlDateTimeString(
      const std::string& sql_date_str) const;  // Or from ODBC timestamp struct

  // Transaction helpers (conceptual)
  void beginTransaction();
  void commitTransaction();
  void rollbackTransaction();
  bool m_in_transaction = false;
};

}  // namespace persistence_service
}  // namespace lms

#endif  // LMS_PERSISTENCE_SERVICE_MS_SQL_PERSISTENCE_SERVICE_H