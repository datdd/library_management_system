#include "persistence_service/file_persistence_service.h"
#include <algorithm>            // For std::replace, std::remove_if
#include <iostream>             // For std::cerr
#include <sstream>              // For std::stringstream
#include "domain_core/book.h"   // For Book specific fields and reconstruction
#include "domain_core/types.h"  // For exceptions

namespace lms {
namespace persistence_service {

// Simple CSV escaping: replace internal commas with a placeholder, and quotes with another
// A proper CSV library would handle this much more robustly.
const char COMMA_PLACEHOLDER = '\x1E';  // Record Separator
const char QUOTE_PLACEHOLDER = '\x1F';  // Unit Separator

std::string FilePersistenceService::escapeCsvField(const std::string& field) const {
  std::string escaped = field;
  std::replace(escaped.begin(), escaped.end(), '"', QUOTE_PLACEHOLDER);
  std::replace(escaped.begin(), escaped.end(), ',', COMMA_PLACEHOLDER);
  return escaped;
}

std::string FilePersistenceService::unescapeCsvField(const std::string& field) const {
  std::string unescaped = field;
  std::replace(unescaped.begin(), unescaped.end(), QUOTE_PLACEHOLDER, '"');
  std::replace(unescaped.begin(), unescaped.end(), COMMA_PLACEHOLDER, ',');
  return unescaped;
}

FilePersistenceService::FilePersistenceService(
    const std::string& data_directory_path,
    std::shared_ptr<utils::DateTimeUtils> date_time_utils)
    : m_data_dir(data_directory_path), m_date_time_utils(std::move(date_time_utils)) {
  if (m_data_dir.empty()) {
    throw InvalidArgumentException(
        "Data directory path cannot be empty for FilePersistenceService.");
  }
  if (!m_date_time_utils) {
    throw InvalidArgumentException("DateTimeUtils cannot be null for FilePersistenceService.");
  }
  // Ensure trailing slash for directory path
  if (m_data_dir.back() != '/' && m_data_dir.back() != '\\') {
    m_data_dir += '/';
  }
  // TODO: Could try to create directory if it doesn't exist.
}

std::vector<std::vector<std::string>> FilePersistenceService::readCsvFile(
    const std::string& filename) const {
  std::vector<std::vector<std::string>> records;
  std::ifstream file(m_data_dir + filename);
  std::string line;

  if (!file.is_open()) {
    // File might not exist yet, which is fine for initial load (means no data)
    // std::cerr << "Warning: Could not open file " << (m_data_dir + filename) << " for reading." <<
    // std::endl;
    return records;  // Return empty vector
  }

  while (std::getline(file, line)) {
    if (line.empty())
      continue;  // Skip empty lines
    std::stringstream ss(line);
    std::string field;
    std::vector<std::string> current_record;
    while (std::getline(ss, field, ',')) {
      current_record.push_back(unescapeCsvField(field));
    }
    records.push_back(current_record);
  }
  file.close();
  return records;
}

void FilePersistenceService::writeCsvFile(const std::string& filename,
                                          const std::vector<std::vector<std::string>>& data) {
  std::ofstream file(m_data_dir + filename, std::ios::trunc);  // Truncate to overwrite
  if (!file.is_open()) {
    throw OperationFailedException("Could not open file " + (m_data_dir + filename) +
                                   " for writing.");
  }

  for (const auto& record : data) {
    for (size_t i = 0; i < record.size(); ++i) {
      file << escapeCsvField(record[i]);
      if (i < record.size() - 1) {
        file << ",";
      }
    }
    file << "\n";
  }
  file.close();
}

// For save, update, delete operations, a common pattern is:
// 1. Load all records.
// 2. Modify in memory (add, update, remove).
// 3. Write all records back.
// This is simple but not efficient for large files. More advanced would be targeted updates.

// --- Author Operations ---
// Format: id,name
void FilePersistenceService::saveAuthor(const std::shared_ptr<Author>& author) {
  if (!author)
    return;
  auto all_authors_data = readCsvFile(m_authors_file);
  bool found = false;
  for (auto& record_fields : all_authors_data) {
    if (!record_fields.empty() && record_fields[0] == author->getId()) {
      record_fields[1] = author->getName();  // Update
      found = true;
      break;
    }
  }
  if (!found) {
    all_authors_data.push_back({author->getId(), author->getName()});  // Add new
  }
  writeCsvFile(m_authors_file, all_authors_data);
}

std::optional<std::shared_ptr<Author>> FilePersistenceService::loadAuthor(
    const EntityId& author_id) {
  auto all_authors_data = readCsvFile(m_authors_file);
  for (const auto& fields : all_authors_data) {
    if (fields.size() == 2 && fields[0] == author_id) {
      return std::make_shared<Author>(fields[0], fields[1]);
    }
  }
  return std::nullopt;
}

std::vector<std::shared_ptr<Author>> FilePersistenceService::loadAllAuthors() {
  std::vector<std::shared_ptr<Author>> authors;
  auto all_authors_data = readCsvFile(m_authors_file);
  for (const auto& fields : all_authors_data) {
    if (fields.size() == 2) {
      try {
        authors.push_back(std::make_shared<Author>(fields[0], fields[1]));
      } catch (const InvalidArgumentException& e) {
        std::cerr << "Skipping invalid author record in " << m_authors_file << ": " << e.what()
                  << std::endl;
      }
    }
  }
  return authors;
}

void FilePersistenceService::deleteAuthor(const EntityId& author_id) {
  auto all_authors_data = readCsvFile(m_authors_file);
  all_authors_data.erase(std::remove_if(all_authors_data.begin(), all_authors_data.end(),
                                        [&](const std::vector<std::string>& record_fields) {
                                          return !record_fields.empty() &&
                                                 record_fields[0] == author_id;
                                        }),
                         all_authors_data.end());
  writeCsvFile(m_authors_file, all_authors_data);
}

// --- Library Item (Book) Operations ---
// Format for items.csv (assuming Book for now):
// item_id,type(Book),title,author_id,isbn,publication_year,availability_status(int)
void FilePersistenceService::saveLibraryItem(const ILibraryItem* item) {
  if (!item)
    return;
  const Book* book = dynamic_cast<const Book*>(item);
  if (!book) {  // Only handle Books for now
    std::cerr << "FilePersistenceService: Skipping save for non-Book item type." << std::endl;
    return;
  }

  auto all_items_data = readCsvFile(m_items_file);
  std::vector<std::string> new_fields = {
      book->getId(),
      "Book",
      book->getTitle(),
      book->getAuthor() ? book->getAuthor()->getId() : "",
      book->getIsbn(),
      std::to_string(book->getPublicationYear()),
      std::to_string(static_cast<int>(book->getAvailabilityStatus()))};

  bool found = false;
  for (auto& record_fields : all_items_data) {
    if (!record_fields.empty() && record_fields[0] == book->getId()) {
      record_fields = new_fields;  // Update
      found = true;
      break;
    }
  }
  if (!found) {
    all_items_data.push_back(new_fields);  // Add new
  }
  writeCsvFile(m_items_file, all_items_data);
}

std::optional<std::unique_ptr<ILibraryItem>> FilePersistenceService::loadLibraryItem(
    const EntityId& item_id) {
  auto all_items_data = readCsvFile(m_items_file);
  for (const auto& fields : all_items_data) {
    if (fields.size() >= 2 && fields[0] == item_id) {
      if (fields[1] == "Book" && fields.size() == 7) {
        try {
          std::shared_ptr<Author> author = nullptr;
          if (!fields[3].empty()) {                   // author_id
            auto author_opt = loadAuthor(fields[3]);  // Load author details
            if (author_opt)
              author = author_opt.value();
            else {
              std::cerr << "Warning: Author ID '" << fields[3] << "' not found for book '"
                        << fields[0] << "'." << std::endl;
              // Potentially continue without author or throw error
            }
          }
          if (!author && !fields[3].empty()) {  // If author ID was there but not found
            std::cerr << "FilePersistenceService: Could not load author " << fields[3]
                      << " for book " << fields[0] << std::endl;
            // Depending on policy, could return nullopt or a book with null author
          }

          return std::make_unique<Book>(
              fields[0],                                             // id
              fields[2],                                             // title
              author,                                                // author (shared_ptr)
              fields[4],                                             // isbn
              std::stoi(fields[5]),                                  // year
              static_cast<AvailabilityStatus>(std::stoi(fields[6]))  // status
          );
        } catch (const std::exception& e) {
          std::cerr << "Error parsing book record for ID " << fields[0] << ": " << e.what()
                    << std::endl;
          return std::nullopt;
        }
      }
    }
  }
  return std::nullopt;
}

std::vector<std::unique_ptr<ILibraryItem>> FilePersistenceService::loadAllLibraryItems() {
  std::vector<std::unique_ptr<ILibraryItem>> items;
  auto all_items_data = readCsvFile(m_items_file);
  for (const auto& fields : all_items_data) {
    if (fields.size() >= 2 && fields[1] == "Book" && fields.size() == 7) {  // Basic check for Book
      try {
        std::shared_ptr<Author> author = nullptr;
        if (!fields[3].empty()) {  // author_id
          auto author_opt = loadAuthor(fields[3]);
          if (author_opt)
            author = author_opt.value();
          else {
            std::cerr << "Warning: Author ID '" << fields[3] << "' not found for book '"
                      << fields[0] << "'." << std::endl;
          }
        }
        if (!author && !fields[3].empty()) {
          std::cerr << "FilePersistenceService: Could not load author " << fields[3] << " for book "
                    << fields[0] << std::endl;
        }

        items.push_back(
            std::make_unique<Book>(fields[0], fields[2], author, fields[4], std::stoi(fields[5]),
                                   static_cast<AvailabilityStatus>(std::stoi(fields[6]))));
      } catch (const std::exception& e) {
        std::cerr << "Error parsing book record in " << m_items_file << " for ID " << fields[0]
                  << ": " << e.what() << std::endl;
      }
    }
  }
  return items;
}

void FilePersistenceService::deleteLibraryItem(const EntityId& item_id) {
  auto all_items_data = readCsvFile(m_items_file);
  all_items_data.erase(std::remove_if(all_items_data.begin(), all_items_data.end(),
                                      [&](const std::vector<std::string>& record_fields) {
                                        return !record_fields.empty() &&
                                               record_fields[0] == item_id;
                                      }),
                       all_items_data.end());
  writeCsvFile(m_items_file, all_items_data);
}

// --- User Operations ---
// Format: user_id,name
void FilePersistenceService::saveUser(const User& user) {
  auto all_users_data = readCsvFile(m_users_file);
  bool found = false;
  for (auto& record_fields : all_users_data) {
    if (!record_fields.empty() && record_fields[0] == user.getUserId()) {
      record_fields[1] = user.getName();  // Update
      found = true;
      break;
    }
  }
  if (!found) {
    all_users_data.push_back({user.getUserId(), user.getName()});  // Add new
  }
  writeCsvFile(m_users_file, all_users_data);
}

std::optional<User> FilePersistenceService::loadUser(const EntityId& user_id) {
  auto all_users_data = readCsvFile(m_users_file);
  for (const auto& fields : all_users_data) {
    if (fields.size() == 2 && fields[0] == user_id) {
      try {
        return User(fields[0], fields[1]);
      } catch (const InvalidArgumentException& e) {
        std::cerr << "Skipping invalid user record in " << m_users_file << ": " << e.what()
                  << std::endl;
      }
    }
  }
  return std::nullopt;
}

std::vector<User> FilePersistenceService::loadAllUsers() {
  std::vector<User> users;
  auto all_users_data = readCsvFile(m_users_file);
  for (const auto& fields : all_users_data) {
    if (fields.size() == 2) {
      try {
        users.emplace_back(fields[0], fields[1]);
      } catch (const InvalidArgumentException& e) {
        std::cerr << "Skipping invalid user record in " << m_users_file << ": " << e.what()
                  << std::endl;
      }
    }
  }
  return users;
}

void FilePersistenceService::deleteUser(const EntityId& user_id) {
  auto all_users_data = readCsvFile(m_users_file);
  all_users_data.erase(std::remove_if(all_users_data.begin(), all_users_data.end(),
                                      [&](const std::vector<std::string>& record_fields) {
                                        return !record_fields.empty() &&
                                               record_fields[0] == user_id;
                                      }),
                       all_users_data.end());
  writeCsvFile(m_users_file, all_users_data);
}

// --- Loan Record Operations ---
// Format: record_id,item_id,user_id,loan_date(YYYY-MM-DD HH:MM:SS),due_date(YYYY-MM-DD
// HH:MM:SS),return_date(optional, YYYY-MM-DD HH:MM:SS or empty)
void FilePersistenceService::saveLoanRecord(const LoanRecord& record) {
  updateLoanRecord(record);  // Same logic for save or update: read all, modify/add, write all
}

void FilePersistenceService::updateLoanRecord(const LoanRecord& record) {
  auto all_loans_data = readCsvFile(m_loans_file);
  std::string return_date_str =
      record.getReturnDate().has_value()
          ? m_date_time_utils->formatDateTime(record.getReturnDate().value())
          : "";
  std::vector<std::string> new_fields = {record.getRecordId(),
                                         record.getItemId(),
                                         record.getUserId(),
                                         m_date_time_utils->formatDateTime(record.getLoanDate()),
                                         m_date_time_utils->formatDateTime(record.getDueDate()),
                                         return_date_str};

  bool found = false;
  for (auto& record_fields : all_loans_data) {
    if (!record_fields.empty() && record_fields[0] == record.getRecordId()) {
      record_fields = new_fields;  // Update
      found = true;
      break;
    }
  }
  if (!found) {
    all_loans_data.push_back(new_fields);  // Add new
  }
  writeCsvFile(m_loans_file, all_loans_data);
}

std::optional<LoanRecord> FilePersistenceService::loadLoanRecord(const EntityId& record_id) {
  auto all_loans_data = readCsvFile(m_loans_file);
  for (const auto& fields : all_loans_data) {
    if (fields.size() == 6 && fields[0] == record_id) {
      try {
        auto loan_date_opt =
            m_date_time_utils->parseDate(fields[3], "%Y-%m-%d %H:%M:%S");  // Use full format
        auto due_date_opt = m_date_time_utils->parseDate(fields[4], "%Y-%m-%d %H:%M:%S");
        if (!loan_date_opt || !due_date_opt) {
          std::cerr << "Error parsing dates for loan record " << fields[0] << std::endl;
          continue;
        }
        LoanRecord loan(fields[0], fields[1], fields[2], loan_date_opt.value(),
                        due_date_opt.value());
        if (!fields[5].empty()) {
          auto return_date_opt = m_date_time_utils->parseDate(fields[5], "%Y-%m-%d %H:%M:%S");
          if (return_date_opt)
            loan.setReturnDate(return_date_opt.value());
          else
            std::cerr << "Error parsing return date for loan record " << fields[0] << std::endl;
        }
        return loan;
      } catch (const std::exception& e) {
        std::cerr << "Error parsing loan record for ID " << fields[0] << ": " << e.what()
                  << std::endl;
      }
    }
  }
  return std::nullopt;
}

std::vector<LoanRecord> FilePersistenceService::loadAllLoanRecords() {
  std::vector<LoanRecord> loans;
  auto all_loans_data = readCsvFile(m_loans_file);
  for (const auto& fields : all_loans_data) {
    if (fields.size() == 6) {  // Expect 6 fields
      try {
        auto loan_date_opt = m_date_time_utils->parseDate(fields[3], "%Y-%m-%d %H:%M:%S");
        auto due_date_opt = m_date_time_utils->parseDate(fields[4], "%Y-%m-%d %H:%M:%S");

        if (!loan_date_opt.has_value() || !due_date_opt.has_value()) {
          std::cerr << "Skipping loan record due to invalid date format: " << fields[0]
                    << std::endl;
          continue;
        }
        LoanRecord loan(fields[0], fields[1], fields[2], loan_date_opt.value(),
                        due_date_opt.value());
        if (!fields[5].empty()) {  // return_date is optional
          auto return_date_opt = m_date_time_utils->parseDate(fields[5], "%Y-%m-%d %H:%M:%S");
          if (return_date_opt.has_value()) {
            loan.setReturnDate(return_date_opt.value());
          } else {
            std::cerr << "Skipping return date for loan record due to invalid format: " << fields[0]
                      << std::endl;
          }
        }
        loans.push_back(loan);
      } catch (const std::exception& e) {  // Catches stoi, etc. and custom exceptions
        std::cerr << "Skipping loan record due to parsing error: " << fields[0] << " - " << e.what()
                  << std::endl;
      }
    } else if (!fields.empty()) {  // Log if not empty but wrong field count
      std::cerr << "Skipping malformed loan record (field count != 6): " << fields[0] << std::endl;
    }
  }
  return loans;
}

std::vector<LoanRecord> FilePersistenceService::loadLoanRecordsByUserId(const EntityId& user_id) {
  std::vector<LoanRecord> user_loans;
  auto all_loans = loadAllLoanRecords();  // Leverage existing method
  for (const auto& loan : all_loans) {
    if (loan.getUserId() == user_id) {
      user_loans.push_back(loan);
    }
  }
  return user_loans;
}

std::vector<LoanRecord> FilePersistenceService::loadLoanRecordsByItemId(const EntityId& item_id) {
  std::vector<LoanRecord> item_loans;
  auto all_loans = loadAllLoanRecords();  // Leverage existing method
  for (const auto& loan : all_loans) {
    if (loan.getItemId() == item_id) {
      item_loans.push_back(loan);
    }
  }
  return item_loans;
}

void FilePersistenceService::deleteLoanRecord(const EntityId& record_id) {
  auto all_loans_data = readCsvFile(m_loans_file);
  all_loans_data.erase(std::remove_if(all_loans_data.begin(), all_loans_data.end(),
                                      [&](const std::vector<std::string>& record_fields) {
                                        return !record_fields.empty() &&
                                               record_fields[0] == record_id;
                                      }),
                       all_loans_data.end());
  writeCsvFile(m_loans_file, all_loans_data);
}

}  // namespace persistence_service
}  // namespace lms