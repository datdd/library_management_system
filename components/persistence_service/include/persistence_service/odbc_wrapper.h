#ifndef ODBC_WRAPPER_H
#define ODBC_WRAPPER_H

#include <map>        // For ResultSetRow by name
#include <memory>     // For unique_ptr
#include <stdexcept>  // For runtime_error
#include <string>
#include <vector>

// Standard ODBC headers
#include <sql.h>
#include <sqlext.h>

namespace odbc_wrapper {

// Custom exception for ODBC errors
class OdbcException : public std::runtime_error {
public:
  OdbcException(const std::string& message, SQLSMALLINT handle_type, SQLHANDLE handle);
  // Helper to format ODBC diagnostic messages
  static std::string GetDiagMessages(SQLSMALLINT handle_type, SQLHANDLE handle);
};

class ResultSet;  // Forward declaration

class PreparedStatement {
public:
  // PreparedStatement takes ownership of the SQLHSTMT
  PreparedStatement(SQLHDBC hdbc, const std::string& sql_query);
  ~PreparedStatement();

  // Disable copy/move until proper resource management is in place for SQLHSTMT
  PreparedStatement(const PreparedStatement&) = delete;
  PreparedStatement& operator=(const PreparedStatement&) = delete;
  PreparedStatement(PreparedStatement&&) = delete;  // Could be implemented with proper move
  PreparedStatement& operator=(PreparedStatement&&) = delete;  // Could be implemented

  void bindString(SQLUSMALLINT param_index, const std::string& value);
  void bindInt(SQLUSMALLINT param_index, int value);
  void bindNull(SQLUSMALLINT param_index,
                SQLSMALLINT sql_type = SQL_VARCHAR);  // Default to VARCHAR for NULL

  std::unique_ptr<ResultSet> executeQuery();
  SQLLEN executeUpdate();  // Returns rows affected

private:
  SQLHDBC m_hdbc;  // Keep a reference to the connection handle for some operations if needed
  SQLHSTMT m_hstmt;
  std::string m_sql;
  int m_param_count;  // Track next parameter index

  void checkRc(SQLRETURN rc, const std::string& operation_desc);
};

class ResultSet {
public:
  // ResultSet takes ownership of the SQLHSTMT from PreparedStatement after query execution
  explicit ResultSet(SQLHSTMT hstmt_executed);
  ~ResultSet();

  // Disable copy/move until proper resource management for SQLHSTMT
  ResultSet(const ResultSet&) = delete;
  ResultSet& operator=(const ResultSet&) = delete;
  ResultSet(ResultSet&&) = delete;
  ResultSet& operator=(ResultSet&&) = delete;

  bool next();  // Advances to the next row, returns false if no more rows

  std::string getString(SQLUSMALLINT col_index);
  std::string getString(const std::string& col_name);
  int getInt(SQLUSMALLINT col_index);
  int getInt(const std::string& col_name);
  bool isNull(SQLUSMALLINT col_index);
  bool isNull(const std::string& col_name);
  // Add more get<Type> methods as needed

  SQLUSMALLINT getColumnCount() const { return m_num_cols; }
  std::string getColumnName(SQLUSMALLINT col_index) const;

private:
  SQLHSTMT m_hstmt;
  SQLSMALLINT m_num_cols;
  std::map<std::string, SQLUSMALLINT> m_col_name_to_index;  // For lookup by name

  void fetchColumnInfo();
  void checkRc(SQLRETURN rc, const std::string& operation_desc);
};

class Connection {
public:
  explicit Connection(const std::string& connection_string);
  ~Connection();

  // Disable copy/move
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;
  Connection(Connection&&) = delete;
  Connection& operator=(Connection&&) = delete;

  bool connect();
  void disconnect();
  bool isConnected() const;

  std::unique_ptr<PreparedStatement> prepareStatement(const std::string& sql_query);

  void beginTransaction();
  void commitTransaction();
  void rollbackTransaction();

private:
  std::string m_connection_string;
  SQLHENV m_henv;  // Environment handle
  SQLHDBC m_hdbc;  // Connection handle
  bool m_is_connected;
  bool m_in_transaction;

  void checkRc(SQLRETURN rc,
               const std::string& operation_desc,
               SQLSMALLINT handle_type,
               SQLHANDLE handle);
};

}  // namespace odbc_wrapper

#endif  // ODBC_WRAPPER_H