#include "persistence_service/ms_sql_persistence_service.h"
#include <iostream>   // For std::cerr for warnings
#include <stdexcept>  // For std::runtime_error (base for LmsException)
#include "domain_core/book.h"
#include "domain_core/types.h"  // For exceptions

// --- Include your chosen ODBC wrapper's headers ---
#include "persistence_service/odbc_wrapper.h"  // Assuming this is the path

namespace lms {
namespace persistence_service {

// Constructor and Destructor
MsSqlPersistenceService::MsSqlPersistenceService(
    const std::string& connection_string,
    std::shared_ptr<utils::DateTimeUtils> date_time_utils)
    : m_connection_string(connection_string),
      m_date_time_utils(std::move(date_time_utils)),
      m_in_transaction(false) {
  if (m_connection_string.empty()) {
    throw InvalidArgumentException("Connection string cannot be empty.");
  }
  if (!m_date_time_utils) {
    throw InvalidArgumentException("DateTimeUtils cannot be null.");
  }
  // Connection is established lazily by getConnection()
}

MsSqlPersistenceService::~MsSqlPersistenceService() {
  if (m_db_connection && m_db_connection->isConnected()) {
    if (m_in_transaction) {  // Rollback any uncommitted transaction
      try {
        std::cerr
            << "MsSqlPersistenceService: Rolling back uncommitted transaction during destruction."
            << std::endl;
        m_db_connection->rollbackTransaction();
      } catch (const odbc_wrapper::OdbcException& e) {
        std::cerr << "MsSqlPersistenceService: Error rolling back transaction during destruction: "
                  << e.what() << std::endl;
      }
    }
    m_db_connection->disconnect();
  }
}

// Private Helper Methods
odbc_wrapper::Connection& MsSqlPersistenceService::getConnection() {
  std::lock_guard<std::mutex> lock(m_db_mutex);
  if (!m_db_connection || !m_db_connection->isConnected()) {
    m_db_connection = std::make_unique<odbc_wrapper::Connection>(m_connection_string);
    if (!m_db_connection
             ->connect()) {  // connect() now part of Connection constructor or explicit call
      throw OperationFailedException("Failed to connect to MS SQL database using: " +
                                     m_connection_string);
    }
  }
  return *m_db_connection;
}

std::string MsSqlPersistenceService::toSqlDateTimeString(const Date& date) const {
  return m_date_time_utils->formatDateTime(date, "%Y-%m-%d %H:%M:%S.%f");  // Include microseconds
}

Date MsSqlPersistenceService::fromSqlDateTimeString(const std::string& sql_date_str) const {
  // ODBC SQL_TIMESTAMP_STRUCT is YYYY-MM-DD HH:MM:SS.fffffffff (9 fractional digits)
  // Our formatDateTime with %f gives 6. We need to handle parsing this carefully.
  // std::get_time doesn't handle fractional seconds.
  // For simplicity, truncating. A robust solution needs a better date/time parser for this.
  std::string to_parse = sql_date_str;
  if (sql_date_str.length() > 19) {  // Contains fractional seconds
    size_t dot_pos = sql_date_str.find('.');
    if (dot_pos != std::string::npos) {
      to_parse = sql_date_str.substr(0, dot_pos);  // Truncate fractional seconds for std::get_time
    } else {
      to_parse = sql_date_str.substr(0, 19);  // Assume it's at least YYYY-MM-DD HH:MM:SS
    }
  }

  auto date_opt = m_date_time_utils->parseDate(to_parse, "%Y-%m-%d %H:%M:%S");
  if (!date_opt) {
    throw OperationFailedException("Failed to parse date string from SQL: '" + sql_date_str +
                                   "' using format '%Y-%m-%d %H:%M:%S' on '" + to_parse + "'");
  }
  return date_opt.value();
}

void MsSqlPersistenceService::beginTransaction() {
  getConnection().beginTransaction();
  m_in_transaction = true;
}
void MsSqlPersistenceService::commitTransaction() {
  if (m_in_transaction) {
    getConnection().commitTransaction();
    m_in_transaction = false;
  }
}
void MsSqlPersistenceService::rollbackTransaction() {
  if (m_in_transaction) {
    try {
      getConnection().rollbackTransaction();
    } catch (const odbc_wrapper::OdbcException& e) {
      std::cerr << "MsSqlPersistenceService: Error during explicit transaction rollback: "
                << e.what() << std::endl;
    }
    m_in_transaction = false;
  }
}

// --- Author Operations ---
void MsSqlPersistenceService::saveAuthor(const std::shared_ptr<Author>& author) {
  if (!author)
    return;
  try {
    std::string sql =
        "MERGE INTO Authors AS Target "
        "USING (VALUES (?, ?)) AS Source (AuthorId_Param, Name_Param) "  // Using _Param to avoid
                                                                         // SQL keyword clash
                                                                         // potential
        "ON Target.AuthorId = Source.AuthorId_Param "
        "WHEN MATCHED THEN UPDATE SET Name = Source.Name_Param "
        "WHEN NOT MATCHED THEN INSERT (AuthorId, Name) VALUES (Source.AuthorId_Param, "
        "Source.Name_Param);";

    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, author->getId());
    stmt_ptr->bindString(2, author->getName());
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error saving author " + author->getId() + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to save author " + author->getId() + ": " + e.what());
  }
}

std::optional<std::shared_ptr<Author>> MsSqlPersistenceService::loadAuthor(
    const EntityId& author_id) {
  try {
    std::string sql = "SELECT AuthorId, Name FROM Authors WHERE AuthorId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, author_id);
    auto rs_ptr = stmt_ptr->executeQuery();

    if (rs_ptr && rs_ptr->next()) {
      return std::make_shared<Author>(rs_ptr->getString("AuthorId"), rs_ptr->getString("Name"));
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error loading author " + author_id + ": " + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to load author " + author_id + ": " + e.what());
  }
  return std::nullopt;
}

std::vector<std::shared_ptr<Author>> MsSqlPersistenceService::loadAllAuthors() {
  std::vector<std::shared_ptr<Author>> authors;
  try {
    std::string sql = "SELECT AuthorId, Name FROM Authors;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    auto rs_ptr = stmt_ptr->executeQuery();

    if (rs_ptr) {
      while (rs_ptr->next()) {
        try {
          authors.push_back(
              std::make_shared<Author>(rs_ptr->getString("AuthorId"), rs_ptr->getString("Name")));
        } catch (const InvalidArgumentException& domain_ex) {
          std::cerr << "MsSqlPersistenceService: Skipping invalid author from DB: "
                    << domain_ex.what() << std::endl;
        }
      }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException(std::string("DB error loading all authors: ") + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException(std::string("Failed to load all authors: ") + e.what());
  }
  return authors;
}

void MsSqlPersistenceService::deleteAuthor(const EntityId& author_id) {
  try {
    std::string sql = "DELETE FROM Authors WHERE AuthorId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, author_id);
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error deleting author " + author_id + ": " + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to delete author " + author_id + ": " + e.what());
  }
}

// --- LibraryItem (Book) Operations ---
void MsSqlPersistenceService::saveLibraryItem(const ILibraryItem* item) {
  if (!item)
    return;
  const Book* book = dynamic_cast<const Book*>(item);
  if (!book) {
    throw InvalidArgumentException("MsSqlPersistenceService currently only supports saving Books.");
  }

  try {
    std::string sql =
        "MERGE INTO LibraryItems AS Target "
        "USING (VALUES (?, ?, ?, ?, ?, ?, ?)) AS Source (ItemId_Param, ItemType_Param, "
        "Title_Param, AuthorId_Param, ISBN_Param, PublicationYear_Param, AvailabilityStatus_Param) "
        "ON Target.ItemId = Source.ItemId_Param "
        "WHEN MATCHED THEN UPDATE SET ItemType = Source.ItemType_Param, Title = "
        "Source.Title_Param, AuthorId = Source.AuthorId_Param, ISBN = Source.ISBN_Param, "
        "PublicationYear = Source.PublicationYear_Param, AvailabilityStatus = "
        "Source.AvailabilityStatus_Param "
        "WHEN NOT MATCHED THEN INSERT (ItemId, ItemType, Title, AuthorId, ISBN, PublicationYear, "
        "AvailabilityStatus) VALUES (Source.ItemId_Param, Source.ItemType_Param, "
        "Source.Title_Param, Source.AuthorId_Param, Source.ISBN_Param, "
        "Source.PublicationYear_Param, Source.AvailabilityStatus_Param);";

    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, book->getId());
    stmt_ptr->bindString(2, "Book");
    stmt_ptr->bindString(3, book->getTitle());
    if (book->getAuthor()) {
      stmt_ptr->bindString(4, book->getAuthor()->getId());
    } else {
      stmt_ptr->bindNull(4, SQL_VARCHAR);
    }
    // ISBN can be NULL in schema, so if book->getIsbn() could be empty and that means NULL:
    if (book->getIsbn().empty()) {
      stmt_ptr->bindNull(5, SQL_VARCHAR);
    } else {
      stmt_ptr->bindString(5, book->getIsbn());
    }
    stmt_ptr->bindInt(6, book->getPublicationYear());
    stmt_ptr->bindInt(7, static_cast<int>(book->getAvailabilityStatus()));
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error saving library item " + item->getId() + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to save library item " + item->getId() + ": " +
                                   e.what());
  }
}

std::optional<std::unique_ptr<ILibraryItem>> MsSqlPersistenceService::loadLibraryItem(
    const EntityId& item_id) {
  try {
    std::string sql =
        "SELECT ItemId, ItemType, Title, AuthorId, ISBN, PublicationYear, AvailabilityStatus FROM "
        "LibraryItems WHERE ItemId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, item_id);
    auto rs_ptr = stmt_ptr->executeQuery();

    if (rs_ptr && rs_ptr->next()) {
      std::string item_type_str = rs_ptr->getString("ItemType");
      if (item_type_str == "Book") {
        std::shared_ptr<Author> author = nullptr;
        if (!rs_ptr->isNull("AuthorId")) {  // Check if AuthorId column is NULL
          std::string db_author_id = rs_ptr->getString("AuthorId");
          if (!db_author_id.empty()) {
            auto author_opt = loadAuthor(db_author_id);
            if (author_opt) {
              author = author_opt.value();
            } else {
              std::cerr << "MsSqlPersistenceService Warning: Author ID '" << db_author_id
                        << "' not found for book '" << item_id << "'." << std::endl;
            }
          }
        }

        std::string isbn_val;
        if (!rs_ptr->isNull("ISBN"))
          isbn_val = rs_ptr->getString("ISBN");

        int pub_year = 0;
        if (!rs_ptr->isNull("PublicationYear"))
          pub_year = rs_ptr->getInt("PublicationYear");

        return std::make_unique<Book>(
            rs_ptr->getString("ItemId"), rs_ptr->getString("Title"), author, isbn_val, pub_year,
            static_cast<AvailabilityStatus>(rs_ptr->getInt("AvailabilityStatus")));
      }
      // else if (item_type_str == "Magazine") { /* ... handle magazines ... */ }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error loading library item " + item_id + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {  // Catches domain exceptions from Book constructor too
    throw OperationFailedException("Failed to load library item " + item_id + ": " + e.what());
  }
  return std::nullopt;
}

std::vector<std::unique_ptr<ILibraryItem>> MsSqlPersistenceService::loadAllLibraryItems() {
  std::vector<std::unique_ptr<ILibraryItem>> items;
  try {
    std::string sql =
        "SELECT ItemId, ItemType, Title, AuthorId, ISBN, PublicationYear, AvailabilityStatus FROM "
        "LibraryItems;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    auto rs_ptr = stmt_ptr->executeQuery();

    if (rs_ptr) {
      while (rs_ptr->next()) {
        try {
          std::string item_type_str = rs_ptr->getString("ItemType");
          if (item_type_str == "Book") {
            std::shared_ptr<Author> author = nullptr;
            if (!rs_ptr->isNull("AuthorId")) {
              std::string db_author_id = rs_ptr->getString("AuthorId");
              if (!db_author_id.empty()) {
                auto author_opt = loadAuthor(db_author_id);
                if (author_opt)
                  author = author_opt.value();
                else {
                  std::cerr << "MsSqlPersistenceService Warning: Author ID '" << db_author_id
                            << "' not found for book '" << rs_ptr->getString("ItemId") << "'."
                            << std::endl;
                }
              }
            }
            std::string isbn_val;
            if (!rs_ptr->isNull("ISBN"))
              isbn_val = rs_ptr->getString("ISBN");
            int pub_year = 0;
            if (!rs_ptr->isNull("PublicationYear"))
              pub_year = rs_ptr->getInt("PublicationYear");

            items.push_back(std::make_unique<Book>(
                rs_ptr->getString("ItemId"), rs_ptr->getString("Title"), author, isbn_val, pub_year,
                static_cast<AvailabilityStatus>(rs_ptr->getInt("AvailabilityStatus"))));
          }
        } catch (const std::exception& e) {
          std::cerr << "MsSqlPersistenceService: Error parsing library item record: "
                    << (rs_ptr->isNull("ItemId") ? "UNKNOWN_ID" : rs_ptr->getString("ItemId"))
                    << " - " << e.what() << std::endl;
        }
      }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException(std::string("DB error loading all library items: ") +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException(std::string("Failed to load all library items: ") + e.what());
  }
  return items;
}

void MsSqlPersistenceService::deleteLibraryItem(const EntityId& item_id) {
  try {
    std::string sql = "DELETE FROM LibraryItems WHERE ItemId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, item_id);
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error deleting library item " + item_id + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to delete library item " + item_id + ": " + e.what());
  }
}

// --- User Operations ---
void MsSqlPersistenceService::saveUser(const User& user) {
  try {
    std::string sql =
        "MERGE INTO Users AS Target "
        "USING (VALUES (?, ?)) AS Source (UserId_Param, Name_Param) "
        "ON Target.UserId = Source.UserId_Param "
        "WHEN MATCHED THEN UPDATE SET Name = Source.Name_Param "
        "WHEN NOT MATCHED THEN INSERT (UserId, Name) VALUES (Source.UserId_Param, "
        "Source.Name_Param);";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, user.getUserId());
    stmt_ptr->bindString(2, user.getName());
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error saving user " + user.getUserId() + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to save user " + user.getUserId() + ": " + e.what());
  }
}

std::optional<User> MsSqlPersistenceService::loadUser(const EntityId& user_id) {
  try {
    std::string sql = "SELECT UserId, Name FROM Users WHERE UserId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, user_id);
    auto rs_ptr = stmt_ptr->executeQuery();
    if (rs_ptr && rs_ptr->next()) {
      return User(rs_ptr->getString("UserId"), rs_ptr->getString("Name"));
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error loading user " + user_id + ": " + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to load user " + user_id + ": " + e.what());
  }
  return std::nullopt;
}

std::vector<User> MsSqlPersistenceService::loadAllUsers() {
  std::vector<User> users;
  try {
    std::string sql = "SELECT UserId, Name FROM Users;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    auto rs_ptr = stmt_ptr->executeQuery();
    if (rs_ptr) {
      while (rs_ptr->next()) {
        try {
          std::string user_id = rs_ptr->getString("UserId");
          std::string user_name = rs_ptr->getString("Name");
          users.emplace_back(user_id, user_name);
          // users.emplace_back(rs_ptr->getString(1), rs_ptr->getString(2));
        } catch (const InvalidArgumentException& domain_ex) {
          std::cerr << "MsSqlPersistenceService: Skipping invalid user from DB: "
                    << domain_ex.what() << std::endl;
        }
      }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException(std::string("DB error loading all users: ") + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException(std::string("Failed to load all users: ") + e.what());
  }
  return users;
}

void MsSqlPersistenceService::deleteUser(const EntityId& user_id) {
  try {
    std::string sql = "DELETE FROM Users WHERE UserId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, user_id);
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error deleting user " + user_id + ": " + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to delete user " + user_id + ": " + e.what());
  }
}

// --- LoanRecord Operations ---
void MsSqlPersistenceService::saveLoanRecord(const LoanRecord& record) {
  updateLoanRecord(record);  // MERGE handles both insert and update
}

void MsSqlPersistenceService::updateLoanRecord(const LoanRecord& record) {
  try {
    std::string sql =
        "MERGE INTO LoanRecords AS Target "
        "USING (VALUES (?, ?, ?, ?, ?, ?)) AS Source (LoanRecordId_Param, ItemId_Param, "
        "UserId_Param, LoanDate_Param, DueDate_Param, ReturnDate_Param) "
        "ON Target.LoanRecordId = Source.LoanRecordId_Param "
        "WHEN MATCHED THEN UPDATE SET ItemId = Source.ItemId_Param, UserId = Source.UserId_Param, "
        "LoanDate = Source.LoanDate_Param, DueDate = Source.DueDate_Param, ReturnDate = "
        "Source.ReturnDate_Param "
        "WHEN NOT MATCHED THEN INSERT (LoanRecordId, ItemId, UserId, LoanDate, DueDate, "
        "ReturnDate) VALUES (Source.LoanRecordId_Param, Source.ItemId_Param, Source.UserId_Param, "
        "Source.LoanDate_Param, Source.DueDate_Param, Source.ReturnDate_Param);";

    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, record.getRecordId());
    stmt_ptr->bindString(2, record.getItemId());
    stmt_ptr->bindString(3, record.getUserId());
    stmt_ptr->bindString(4, toSqlDateTimeString(record.getLoanDate()));
    stmt_ptr->bindString(5, toSqlDateTimeString(record.getDueDate()));
    if (record.getReturnDate().has_value()) {
      stmt_ptr->bindString(6, toSqlDateTimeString(record.getReturnDate().value()));
    } else {
      stmt_ptr->bindNull(6, SQL_TYPE_TIMESTAMP);  // Use appropriate SQL type for DATETIME2
    }
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error saving/updating loan record " + record.getRecordId() +
                                   ": " + db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to save/update loan record " + record.getRecordId() +
                                   ": " + e.what());
  }
}

std::optional<LoanRecord> MsSqlPersistenceService::loadLoanRecord(const EntityId& record_id) {
  try {
    std::string sql =
        "SELECT LoanRecordId, ItemId, UserId, LoanDate, DueDate, ReturnDate FROM LoanRecords WHERE "
        "LoanRecordId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, record_id);
    auto rs_ptr = stmt_ptr->executeQuery();

    if (rs_ptr && rs_ptr->next()) {
      Date loan_date = fromSqlDateTimeString(rs_ptr->getString("LoanDate"));
      Date due_date = fromSqlDateTimeString(rs_ptr->getString("DueDate"));
      LoanRecord loan(rs_ptr->getString("LoanRecordId"), rs_ptr->getString("ItemId"),
                      rs_ptr->getString("UserId"), loan_date, due_date);

      if (!rs_ptr->isNull("ReturnDate")) {
        std::string return_date_str = rs_ptr->getString("ReturnDate");
        if (!return_date_str.empty()) {
          loan.setReturnDate(fromSqlDateTimeString(return_date_str));
        }
      }
      return loan;
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error loading loan record " + record_id + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {  // Catches domain or parsing exceptions
    throw OperationFailedException("Error parsing loan record for ID " + record_id + ": " +
                                   e.what());
  }
  return std::nullopt;
}

std::vector<LoanRecord> MsSqlPersistenceService::loadAllLoanRecords() {
  std::vector<LoanRecord> loans;
  try {
    std::string sql =
        "SELECT LoanRecordId, ItemId, UserId, LoanDate, DueDate, ReturnDate FROM LoanRecords;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    auto rs_ptr = stmt_ptr->executeQuery();
    if (rs_ptr) {
      while (rs_ptr->next()) {
        try {
          Date loan_date = fromSqlDateTimeString(rs_ptr->getString("LoanDate"));
          Date due_date = fromSqlDateTimeString(rs_ptr->getString("DueDate"));
          LoanRecord loan(rs_ptr->getString("LoanRecordId"), rs_ptr->getString("ItemId"),
                          rs_ptr->getString("UserId"), loan_date, due_date);
          if (!rs_ptr->isNull("ReturnDate")) {
            std::string return_date_str = rs_ptr->getString("ReturnDate");
            if (!return_date_str.empty()) {
              loan.setReturnDate(fromSqlDateTimeString(return_date_str));
            }
          }
          loans.push_back(loan);
        } catch (const std::exception& parse_ex) {
          std::cerr << "MsSqlPersistenceService: Skipping loan record due to parsing error: "
                    << (rs_ptr->isNull("LoanRecordId") ? "UNKNOWN_ID"
                                                       : rs_ptr->getString("LoanRecordId"))
                    << " - " << parse_ex.what() << std::endl;
        }
      }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException(std::string("DB error loading all loan records: ") +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException(std::string("Failed to load all loan records: ") + e.what());
  }
  return loans;
}

std::vector<LoanRecord> MsSqlPersistenceService::loadLoanRecordsByUserId(const EntityId& user_id) {
  std::vector<LoanRecord> loans;
  try {
    std::string sql =
        "SELECT LoanRecordId, ItemId, UserId, LoanDate, DueDate, ReturnDate FROM LoanRecords WHERE "
        "UserId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, user_id);
    auto rs_ptr = stmt_ptr->executeQuery();
    if (rs_ptr) {
      while (rs_ptr->next()) {
        // ... (same parsing logic as loadAllLoanRecords) ...
        try {
          Date loan_date = fromSqlDateTimeString(rs_ptr->getString("LoanDate"));
          Date due_date = fromSqlDateTimeString(rs_ptr->getString("DueDate"));
          LoanRecord loan(rs_ptr->getString("LoanRecordId"), rs_ptr->getString("ItemId"),
                          rs_ptr->getString("UserId"), loan_date, due_date);
          if (!rs_ptr->isNull("ReturnDate")) {
            std::string return_date_str = rs_ptr->getString("ReturnDate");
            if (!return_date_str.empty()) {
              loan.setReturnDate(fromSqlDateTimeString(return_date_str));
            }
          }
          loans.push_back(loan);
        } catch (const std::exception& parse_ex) {
          std::cerr << "MsSqlPersistenceService: Skipping loan record for user " << user_id
                    << " due to parsing error: "
                    << (rs_ptr->isNull("LoanRecordId") ? "UNKNOWN_ID"
                                                       : rs_ptr->getString("LoanRecordId"))
                    << " - " << parse_ex.what() << std::endl;
        }
      }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error loading loans for user " + user_id + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to load loans for user " + user_id + ": " + e.what());
  }
  return loans;
}

std::vector<LoanRecord> MsSqlPersistenceService::loadLoanRecordsByItemId(const EntityId& item_id) {
  std::vector<LoanRecord> loans;
  try {
    std::string sql =
        "SELECT LoanRecordId, ItemId, UserId, LoanDate, DueDate, ReturnDate FROM LoanRecords WHERE "
        "ItemId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, item_id);
    auto rs_ptr = stmt_ptr->executeQuery();
    if (rs_ptr) {
      while (rs_ptr->next()) {
        // ... (same parsing logic as loadAllLoanRecords) ...
        try {
          Date loan_date = fromSqlDateTimeString(rs_ptr->getString("LoanDate"));
          Date due_date = fromSqlDateTimeString(rs_ptr->getString("DueDate"));
          LoanRecord loan(rs_ptr->getString("LoanRecordId"), rs_ptr->getString("ItemId"),
                          rs_ptr->getString("UserId"), loan_date, due_date);
          if (!rs_ptr->isNull("ReturnDate")) {
            std::string return_date_str = rs_ptr->getString("ReturnDate");
            if (!return_date_str.empty()) {
              loan.setReturnDate(fromSqlDateTimeString(return_date_str));
            }
          }
          loans.push_back(loan);
        } catch (const std::exception& parse_ex) {
          std::cerr << "MsSqlPersistenceService: Skipping loan record for item " << item_id
                    << " due to parsing error: "
                    << (rs_ptr->isNull("LoanRecordId") ? "UNKNOWN_ID"
                                                       : rs_ptr->getString("LoanRecordId"))
                    << " - " << parse_ex.what() << std::endl;
        }
      }
    }
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error loading loans for item " + item_id + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to load loans for item " + item_id + ": " + e.what());
  }
  return loans;
}

void MsSqlPersistenceService::deleteLoanRecord(const EntityId& record_id) {
  try {
    std::string sql = "DELETE FROM LoanRecords WHERE LoanRecordId = ?;";
    auto stmt_ptr = getConnection().prepareStatement(sql);
    stmt_ptr->bindString(1, record_id);
    stmt_ptr->executeUpdate();
  } catch (const odbc_wrapper::OdbcException& db_ex) {
    throw OperationFailedException("DB error deleting loan record " + record_id + ": " +
                                   db_ex.what());
  } catch (const std::exception& e) {
    throw OperationFailedException("Failed to delete loan record " + record_id + ": " + e.what());
  }
}

}  // namespace persistence_service
}  // namespace lms