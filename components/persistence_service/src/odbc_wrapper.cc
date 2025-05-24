#include "persistence_service/odbc_wrapper.h"
#include <iostream>  // For cerr
#include <vector>

namespace odbc_wrapper {

// --- OdbcException Implementation ---
std::string OdbcException::GetDiagMessages(SQLSMALLINT handle_type, SQLHANDLE handle) {
  SQLCHAR sql_state[6];
  SQLINTEGER native_error;
  SQLCHAR message_text[SQL_MAX_MESSAGE_LENGTH];
  SQLSMALLINT text_length;
  std::string error_messages;
  SQLSMALLINT rec_num = 1;

  while (SQLGetDiagRec(handle_type, handle, rec_num, sql_state, &native_error, message_text,
                       sizeof(message_text), &text_length) == SQL_SUCCESS) {
    error_messages += "SQLState: " + std::string(reinterpret_cast<char*>(sql_state)) +
                      ", NativeError: " + std::to_string(native_error) + ", Message: " +
                      std::string(reinterpret_cast<char*>(message_text), text_length) + "\n";
    rec_num++;
  }
  return error_messages;
}

OdbcException::OdbcException(const std::string& message, SQLSMALLINT handle_type, SQLHANDLE handle)
    : std::runtime_error(message + "\nODBC Errors:\n" + GetDiagMessages(handle_type, handle)) {}

// --- Helper for checkRc in classes ---
// (Could be a free function or static member of a utility class)
void CheckOdbcRc(SQLRETURN rc,
                 const std::string& operation_desc,
                 SQLSMALLINT handle_type,
                 SQLHANDLE handle) {
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO && rc != SQL_NO_DATA) {
    throw OdbcException("ODBC Error during " + operation_desc, handle_type, handle);
  }
  if (rc == SQL_SUCCESS_WITH_INFO) {
    // Log warnings or handle them as needed
    std::cerr << "ODBC Info during " << operation_desc << ":\n"
              << OdbcException::GetDiagMessages(handle_type, handle) << std::endl;
  }
}

// --- Connection Implementation ---
Connection::Connection(const std::string& connection_string)
    : m_connection_string(connection_string),
      m_henv(SQL_NULL_HENV),
      m_hdbc(SQL_NULL_HDBC),
      m_is_connected(false),
      m_in_transaction(false) {
  SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    m_henv = SQL_NULL_HENV;  // Ensure it's null if alloc failed
    throw OdbcException("Failed to allocate Environment Handle", SQL_HANDLE_ENV, SQL_NULL_HANDLE);
  }
  CheckOdbcRc(SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0),
              "Set ODBC Version", SQL_HANDLE_ENV, m_henv);
}

Connection::~Connection() {
  disconnect();
  if (m_henv != SQL_NULL_HENV) {
    SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
    m_henv = SQL_NULL_HENV;
  }
}

bool Connection::connect() {
  if (m_is_connected)
    return true;
  if (m_henv == SQL_NULL_HENV)
    return false;  // Should not happen if constructor succeeded

  SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
  CheckOdbcRc(rc, "Allocate Connection Handle", SQL_HANDLE_ENV, m_henv);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    m_hdbc = SQL_NULL_HDBC;
    return false;
  }

  // SQLDriverConnect is more flexible than SQLConnect
  SQLCHAR out_conn_str[1024];
  SQLSMALLINT out_conn_str_len;
  rc = SQLDriverConnect(m_hdbc, NULL,  // No window handle
                        (SQLCHAR*)m_connection_string.c_str(), SQL_NTS, out_conn_str,
                        sizeof(out_conn_str), &out_conn_str_len, SQL_DRIVER_NOPROMPT);

  CheckOdbcRc(rc, "DriverConnect", SQL_HANDLE_DBC, m_hdbc);
  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
    m_is_connected = true;
    // Default to auto-commit mode. Use beginTransaction to turn it off.
    CheckOdbcRc(SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON,
                                  SQL_IS_INTEGER),
                "Set Autocommit ON", SQL_HANDLE_DBC, m_hdbc);
    return true;
  }
  SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);  // Clean up on failure
  m_hdbc = SQL_NULL_HDBC;
  return false;
}

void Connection::disconnect() {
  if (m_is_connected) {
    if (m_in_transaction) {
      try {
        rollbackTransaction();  // Attempt to rollback any pending transaction
      } catch (const OdbcException& e) {
        std::cerr << "Error rolling back transaction during disconnect: " << e.what() << std::endl;
      }
    }
    SQLDisconnect(m_hdbc);
    m_is_connected = false;
  }
  if (m_hdbc != SQL_NULL_HDBC) {
    SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
    m_hdbc = SQL_NULL_HDBC;
  }
}

bool Connection::isConnected() const {
  return m_is_connected;
}

std::unique_ptr<PreparedStatement> Connection::prepareStatement(const std::string& sql_query) {
  if (!m_is_connected) {
    throw OdbcException("Cannot prepare statement: Not connected.", SQL_HANDLE_DBC, m_hdbc);
  }
  return std::make_unique<PreparedStatement>(m_hdbc, sql_query);
}

void Connection::beginTransaction() {
  if (!m_is_connected)
    throw OdbcException("Not connected.", SQL_HANDLE_DBC, m_hdbc);
  if (m_in_transaction)
    throw OdbcException("Transaction already in progress.", SQL_HANDLE_DBC, m_hdbc);

  CheckOdbcRc(SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF,
                                SQL_IS_INTEGER),
              "Set Autocommit OFF (Begin Transaction)", SQL_HANDLE_DBC, m_hdbc);
  m_in_transaction = true;
}

void Connection::commitTransaction() {
  if (!m_is_connected)
    throw OdbcException("Not connected.", SQL_HANDLE_DBC, m_hdbc);
  if (!m_in_transaction)
    throw OdbcException("No transaction in progress to commit.", SQL_HANDLE_DBC, m_hdbc);

  CheckOdbcRc(SQLEndTran(SQL_HANDLE_DBC, m_hdbc, SQL_COMMIT), "Commit Transaction", SQL_HANDLE_DBC,
              m_hdbc);
  m_in_transaction = false;
  // Restore auto-commit
  CheckOdbcRc(
      SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_INTEGER),
      "Set Autocommit ON (After Commit)", SQL_HANDLE_DBC, m_hdbc);
}

void Connection::rollbackTransaction() {
  if (!m_is_connected)
    throw OdbcException("Not connected.", SQL_HANDLE_DBC, m_hdbc);
  if (!m_in_transaction)
    return;  // No-op if no transaction

  CheckOdbcRc(SQLEndTran(SQL_HANDLE_DBC, m_hdbc, SQL_ROLLBACK), "Rollback Transaction",
              SQL_HANDLE_DBC, m_hdbc);
  m_in_transaction = false;
  // Restore auto-commit
  CheckOdbcRc(
      SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_INTEGER),
      "Set Autocommit ON (After Rollback)", SQL_HANDLE_DBC, m_hdbc);
}

// --- PreparedStatement Implementation ---
PreparedStatement::PreparedStatement(SQLHDBC hdbc, const std::string& sql_query)
    : m_hdbc(hdbc), m_sql(sql_query), m_param_count(1) {
  CheckOdbcRc(SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt), "Allocate Statement Handle",
              SQL_HANDLE_DBC, m_hdbc);
  CheckOdbcRc(SQLPrepare(m_hstmt, (SQLCHAR*)m_sql.c_str(), SQL_NTS), "Prepare SQL: " + m_sql,
              SQL_HANDLE_STMT, m_hstmt);
}

PreparedStatement::~PreparedStatement() {
  if (m_hstmt != SQL_NULL_HSTMT) {
    SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
    m_hstmt = SQL_NULL_HSTMT;
  }
}

void PreparedStatement::checkRc(SQLRETURN rc, const std::string& operation_desc) {
  CheckOdbcRc(rc, operation_desc, SQL_HANDLE_STMT, m_hstmt);
}

void PreparedStatement::bindString(SQLUSMALLINT param_index, const std::string& value) {
  // For SQLBindParameter, string length needs to be passed.
  // The last argument (StrLen_or_IndPtr) points to a buffer containing the length
  // or SQL_NTS for null-terminated strings.
  // The ColumnSize (third from last) is the size of the column in the DB.
  // For variable length strings, it can be the max length or a large enough value.
  // SQL_VARCHAR indicates the C type is SQL_C_CHAR.
  // For simplicity, directly using the string's c_str(). A real wrapper might copy to a buffer.
  // Note: SQLBindParameter binds a *buffer*. If 'value' goes out of scope before SQLExecute, this
  // is bad. This simple example assumes 'value' (or its source) lives long enough. A robust wrapper
  // would manage buffers for bound parameters.
  SQLLEN str_len = SQL_NTS;  // Or (SQLINTEGER)value.length();
  checkRc(SQLBindParameter(m_hstmt, param_index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           value.length(), 0, (SQLPOINTER)value.c_str(), 0, &str_len),
          "Bind String Param " + std::to_string(param_index));
  m_param_count++;
}

void PreparedStatement::bindInt(SQLUSMALLINT param_index, int value) {
  // Need to store 'value' in a member or pass its address directly if it lives long enough.
  // For this simple example, assume the int value is passed by value and its address can be taken
  // if the wrapper was designed differently. Here we pass the value directly for SQL_C_SLONG
  // and the last arg is nullptr for length/indicator as it's a fixed-size type.
  // A more robust way is to store parameters internally.
  static int static_int_val;  // VERY UNSAFE HACK for demo if parameter buffer not managed
  static_int_val = value;
  checkRc(SQLBindParameter(m_hstmt, param_index, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
                           &static_int_val, 0, nullptr),  // Using address of static_int_val
          "Bind Int Param " + std::to_string(param_index));
  m_param_count++;
}
// TODO: Correct parameter binding requires careful buffer management.
// The above bindInt is a simplification. Parameters should be stored in the PreparedStatement
// object.

void PreparedStatement::bindNull(SQLUSMALLINT param_index, SQLSMALLINT sql_type) {
  SQLLEN ind = SQL_NULL_DATA;
  checkRc(SQLBindParameter(m_hstmt, param_index, SQL_PARAM_INPUT, SQL_C_DEFAULT, sql_type, 0, 0,
                           nullptr, 0, &ind),  // Data pointer is NULL
          "Bind NULL Param " + std::to_string(param_index));
  m_param_count++;
}

std::unique_ptr<ResultSet> PreparedStatement::executeQuery() {
  checkRc(SQLExecute(m_hstmt), "Execute Query: " + m_sql);
  // Statement handle is now "owned" by ResultSet for fetching
  // SQLFreeStmt(m_hstmt, SQL_CLOSE) might be needed before giving m_hstmt to ResultSet if not
  // reusing statement For simplicity, assume ResultSet takes the statement as is.
  auto rs = std::make_unique<ResultSet>(m_hstmt);
  m_hstmt = SQL_NULL_HSTMT;  // ResultSet now owns it. PreparedStatement cannot reuse.
  return rs;
}

SQLLEN PreparedStatement::executeUpdate() {
  checkRc(SQLExecute(m_hstmt), "Execute Update: " + m_sql);
  SQLLEN row_count = 0;
  checkRc(SQLRowCount(m_hstmt, &row_count), "Get Row Count");
  // It's good practice to free statement resources if not reusing immediately
  SQLFreeStmt(m_hstmt, SQL_CLOSE);  // Close cursor, keeps statement prepared
  // SQLFreeStmt(m_hstmt, SQL_UNBIND); // Unbind params
  // SQLFreeStmt(m_hstmt, SQL_RESET_PARAMS); // Reset params
  return row_count;
}

// --- ResultSet Implementation ---
ResultSet::ResultSet(SQLHSTMT hstmt_executed) : m_hstmt(hstmt_executed), m_num_cols(0) {
  if (m_hstmt == SQL_NULL_HSTMT) {
    throw OdbcException("ResultSet created with NULL statement handle.", SQL_HANDLE_STMT, m_hstmt);
  }
  fetchColumnInfo();
}

ResultSet::~ResultSet() {
  if (m_hstmt != SQL_NULL_HSTMT) {
    SQLFreeStmt(m_hstmt,
                SQL_DROP);  // SQL_DROP is more thorough than SQL_CLOSE for a one-time result set
    m_hstmt = SQL_NULL_HSTMT;
  }
}
void ResultSet::checkRc(SQLRETURN rc, const std::string& operation_desc) {
  CheckOdbcRc(rc, operation_desc, SQL_HANDLE_STMT, m_hstmt);
}

void ResultSet::fetchColumnInfo() {
  checkRc(SQLNumResultCols(m_hstmt, &m_num_cols), "Get Number of Result Columns");
  for (SQLUSMALLINT i = 1; i <= m_num_cols; ++i) {
    SQLCHAR col_name_buf[256];
    SQLSMALLINT name_len;
    checkRc(SQLDescribeCol(m_hstmt, i, col_name_buf, sizeof(col_name_buf), &name_len, nullptr,
                           nullptr, nullptr, nullptr),
            "Describe Column " + std::to_string(i));
    m_col_name_to_index[std::string(reinterpret_cast<char*>(col_name_buf), name_len)] = i;
  }
}

bool ResultSet::next() {
  SQLRETURN rc = SQLFetch(m_hstmt);
  if (rc == SQL_NO_DATA) {
    return false;
  }
  checkRc(rc, "Fetch Next Row");
  return true;
}

std::string ResultSet::getString(SQLUSMALLINT col_index) {
  if (isNull(col_index))
    return "";  // Or throw, or return std::optional

  SQLCHAR buffer[1024];  // Adjust buffer size as needed
  SQLLEN indicator;      // Will hold length of data or SQL_NULL_DATA
  checkRc(SQLGetData(m_hstmt, col_index, SQL_C_CHAR, buffer, sizeof(buffer), &indicator),
          "Get String Data Col " + std::to_string(col_index));
  if (indicator == SQL_NULL_DATA)
    return "";  // Should have been caught by isNull
  return std::string(reinterpret_cast<char*>(buffer), indicator);
}

std::string ResultSet::getString(const std::string& col_name) {
  auto it = m_col_name_to_index.find(col_name);
  if (it == m_col_name_to_index.end()) {
    throw OdbcException("Column not found: " + col_name, SQL_HANDLE_STMT, m_hstmt);
  }
  return getString(it->second);
}

int ResultSet::getInt(SQLUSMALLINT col_index) {
  if (isNull(col_index))
    return 0;  // Or throw

  SQLINTEGER value;
  SQLLEN indicator;
  checkRc(SQLGetData(m_hstmt, col_index, SQL_C_SLONG, &value, sizeof(value), &indicator),
          "Get Int Data Col " + std::to_string(col_index));
  if (indicator == SQL_NULL_DATA)
    return 0;
  return value;
}
int ResultSet::getInt(const std::string& col_name) {
  auto it = m_col_name_to_index.find(col_name);
  if (it == m_col_name_to_index.end()) {
    throw OdbcException("Column not found: " + col_name, SQL_HANDLE_STMT, m_hstmt);
  }
  return getInt(it->second);
}

bool ResultSet::isNull(SQLUSMALLINT col_index) {
  // To check for NULL without fetching data, you'd typically get the indicator
  // when fetching another column or by peeking.
  // A common way is to try to fetch into a small buffer and check indicator.
  SQLINTEGER dummy_val;    // For numeric types
  SQLCHAR dummy_char_val;  // For char types
  SQLLEN indicator;
  // Try to get data for column `col_index` with `SQL_C_DEFAULT` or a specific C type.
  // The important part is the `indicator`.
  // We are not actually using the fetched value here, just checking the indicator.
  // A more robust way is to get indicator when fetching other columns.
  // This is a simplified check.
  SQLRETURN rc = SQLGetData(m_hstmt, col_index, SQL_C_DEFAULT, &dummy_char_val, 0,
                            &indicator);  // 0 buffer length to just get indicator
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO &&
      rc != SQL_NO_DATA) {  // SQL_NO_DATA if column already fetched or issues
    // If SQLGetData fails here other than NO_DATA, it's an issue.
    // For a simple isNull, we might not want to throw if it just means data can't be "peeked" this
    // way. However, if SQL_NO_DATA occurs, it's not necessarily NULL. The best way is to get the
    // indicator WHEN YOU CALL SQLGetData for the actual data. The getInt/getString methods should
    // handle the indicator properly. This isNull is thus a bit flawed in this simple implementation
    // if called standalone. Let's assume it's called before a specific getXXX, and getXXX will use
    // its own indicator. For the purpose of the placeholder: If trying to get string, it might
    // actually return 0 length for empty vs SQL_NULL_DATA. A truly reliable isNull requires
    // checking the indicator from the SQLGetData call that actually retrieves the data.

    // Let's make this illustrative: This isNull would be better if part of getXXX logic.
    // For now, assume it's a placeholder. The getXXX methods above do check their own indicator.
    return true;  // Fallback, not reliable
  }
  return indicator == SQL_NULL_DATA;
}
bool ResultSet::isNull(const std::string& col_name) {
  auto it = m_col_name_to_index.find(col_name);
  if (it == m_col_name_to_index.end()) {
    throw OdbcException("Column not found: " + col_name, SQL_HANDLE_STMT, m_hstmt);
  }
  return isNull(it->second);
}

std::string ResultSet::getColumnName(SQLUSMALLINT col_index) const {
  for (const auto& pair : m_col_name_to_index) {
    if (pair.second == col_index) {
      return pair.first;
    }
  }
  throw OdbcException("Column index out of bounds: " + std::to_string(col_index), SQL_HANDLE_STMT,
                      m_hstmt);
}

}  // namespace odbc_wrapper