#include "persistence_service/caching_file_persistence_service.h"
#include <iostream>             // For status messages
#include "domain_core/types.h"  // For exceptions

namespace lms {
namespace persistence_service {

CachingFilePersistenceService::CachingFilePersistenceService(
    const std::string& data_directory_path,
    std::shared_ptr<utils::DateTimeUtils>
        date_time_utils) {  // date_time_utils for FilePersistenceService

  m_memory_store = std::make_unique<InMemoryPersistenceService>();
  m_file_store = std::make_unique<FilePersistenceService>(data_directory_path, date_time_utils);

  loadAllFromFileToMemory();
}

void CachingFilePersistenceService::loadAllFromFileToMemory() {
  std::cout << "CachingFilePersistenceService: Loading data from files into memory..." << std::endl;
  // Clear memory store first (or decide on merge strategy)
  // For simplicity, let's assume a full reload.
  m_memory_store = std::make_unique<InMemoryPersistenceService>();

  // Load Authors
  auto authors_from_file = m_file_store->loadAllAuthors();
  for (const auto& author : authors_from_file) {
    m_memory_store->saveAuthor(author);
  }
  std::cout << "Loaded " << authors_from_file.size() << " authors." << std::endl;

  // Load Users
  auto users_from_file = m_file_store->loadAllUsers();
  for (const auto& user : users_from_file) {
    m_memory_store->saveUser(user);
  }
  std::cout << "Loaded " << users_from_file.size() << " users." << std::endl;

  // Load Library Items
  // Note: loadAllLibraryItems returns unique_ptrs. saveLibraryItem takes const ILibraryItem*
  auto items_from_file = m_file_store->loadAllLibraryItems();
  for (const auto& item_ptr : items_from_file) {
    if (item_ptr) {
      m_memory_store->saveLibraryItem(item_ptr.get());
    }
  }
  std::cout << "Loaded " << items_from_file.size() << " library items." << std::endl;

  // Load Loan Records
  auto loans_from_file = m_file_store->loadAllLoanRecords();
  for (const auto& loan : loans_from_file) {
    m_memory_store->saveLoanRecord(loan);
  }
  std::cout << "Loaded " << loans_from_file.size() << " loan records." << std::endl;
  std::cout << "CachingFilePersistenceService: Data loading complete." << std::endl;
}

void CachingFilePersistenceService::persistAllToFile() {
  std::cout << "CachingFilePersistenceService: Persisting all in-memory data to files..."
            << std::endl;
  // This will overwrite existing files with the current in-memory state.

  // Save Authors
  auto all_authors_mem = m_memory_store->loadAllAuthors();
  for (const auto& author : all_authors_mem) {
    m_file_store->saveAuthor(author);  // File store's saveAuthor handles overwrite/add
  }
  // To ensure authors not in memory (e.g. deleted from memory but not file) are removed from file,
  // a more robust approach would be to clear the file store's authors first or pass all authors to
  // a 'replaceAll' type method. For simplicity, current file_store save methods do an upsert. A
  // true "sync all from memory to file" would mean deleting file entries not in memory. This simple
  // version just ensures all memory items are in the file.
  std::cout << "Persisted " << all_authors_mem.size() << " authors." << std::endl;

  // Save Users
  auto all_users_mem = m_memory_store->loadAllUsers();
  for (const auto& user : all_users_mem) {
    m_file_store->saveUser(user);
  }
  std::cout << "Persisted " << all_users_mem.size() << " users." << std::endl;

  // Save Library Items
  auto all_items_mem = m_memory_store->loadAllLibraryItems();  // Gets clones
  // To avoid deleting and re-adding all, ideally FilePersistenceService would have a
  // "clearAllItems" For now, this will effectively update/add items from memory to file. Items
  // deleted only from memory won't be deleted from file with this simple loop.
  for (const auto& item_ptr : all_items_mem) {
    if (item_ptr) {
      m_file_store->saveLibraryItem(item_ptr.get());
    }
  }
  std::cout << "Persisted " << all_items_mem.size() << " library items." << std::endl;

  // Save Loan Records
  auto all_loans_mem = m_memory_store->loadAllLoanRecords();
  for (const auto& loan : all_loans_mem) {
    m_file_store->saveLoanRecord(loan);  // or updateLoanRecord
  }
  std::cout << "Persisted " << all_loans_mem.size() << " loan records." << std::endl;
  std::cout << "CachingFilePersistenceService: Data persistence complete." << std::endl;
}

// --- Standard IPersistenceService methods (delegate to m_memory_store) ---
// --- If you want write-through, also call m_file_store here ---

void CachingFilePersistenceService::saveAuthor(const std::shared_ptr<Author>& author) {
  m_memory_store->saveAuthor(author);
  // Optional write-through: m_file_store->saveAuthor(author);
}
std::optional<std::shared_ptr<Author>> CachingFilePersistenceService::loadAuthor(
    const EntityId& author_id) {
  return m_memory_store->loadAuthor(author_id);
}
std::vector<std::shared_ptr<Author>> CachingFilePersistenceService::loadAllAuthors() {
  return m_memory_store->loadAllAuthors();
}
void CachingFilePersistenceService::deleteAuthor(const EntityId& author_id) {
  m_memory_store->deleteAuthor(author_id);
  // Optional write-through: m_file_store->deleteAuthor(author_id);
}

void CachingFilePersistenceService::saveLibraryItem(const ILibraryItem* item) {
  m_memory_store->saveLibraryItem(item);
  // Optional write-through: m_file_store->saveLibraryItem(item);
}
std::optional<std::unique_ptr<ILibraryItem>> CachingFilePersistenceService::loadLibraryItem(
    const EntityId& item_id) {
  return m_memory_store->loadLibraryItem(item_id);
}
std::vector<std::unique_ptr<ILibraryItem>> CachingFilePersistenceService::loadAllLibraryItems() {
  return m_memory_store->loadAllLibraryItems();
}
void CachingFilePersistenceService::deleteLibraryItem(const EntityId& item_id) {
  m_memory_store->deleteLibraryItem(item_id);
  // Optional write-through: m_file_store->deleteLibraryItem(item_id);
}

void CachingFilePersistenceService::saveUser(const User& user) {
  m_memory_store->saveUser(user);
  // Optional write-through: m_file_store->saveUser(user);
}
std::optional<User> CachingFilePersistenceService::loadUser(const EntityId& user_id) {
  return m_memory_store->loadUser(user_id);
}
std::vector<User> CachingFilePersistenceService::loadAllUsers() {
  return m_memory_store->loadAllUsers();
}
void CachingFilePersistenceService::deleteUser(const EntityId& user_id) {
  m_memory_store->deleteUser(user_id);
  // Optional write-through: m_file_store->deleteUser(user_id);
}

void CachingFilePersistenceService::saveLoanRecord(const LoanRecord& record) {
  m_memory_store->saveLoanRecord(record);
  // Optional write-through: m_file_store->saveLoanRecord(record);
}
std::optional<LoanRecord> CachingFilePersistenceService::loadLoanRecord(const EntityId& record_id) {
  return m_memory_store->loadLoanRecord(record_id);
}
std::vector<LoanRecord> CachingFilePersistenceService::loadLoanRecordsByUserId(
    const EntityId& user_id) {
  return m_memory_store->loadLoanRecordsByUserId(user_id);
}
std::vector<LoanRecord> CachingFilePersistenceService::loadLoanRecordsByItemId(
    const EntityId& item_id) {
  return m_memory_store->loadLoanRecordsByItemId(item_id);
}
std::vector<LoanRecord> CachingFilePersistenceService::loadAllLoanRecords() {
  return m_memory_store->loadAllLoanRecords();
}
void CachingFilePersistenceService::deleteLoanRecord(const EntityId& record_id) {
  m_memory_store->deleteLoanRecord(record_id);
  // Optional write-through: m_file_store->deleteLoanRecord(record_id);
}
void CachingFilePersistenceService::updateLoanRecord(const LoanRecord& record) {
  m_memory_store->updateLoanRecord(record);
  // Optional write-through: m_file_store->updateLoanRecord(record);
}

}  // namespace persistence_service
}  // namespace lms