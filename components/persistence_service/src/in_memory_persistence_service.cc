#include "persistence_service/in_memory_persistence_service.h"
#include "domain_core/types.h" // For exceptions like NotFoundException

#include <algorithm> // for std::find_if, std::remove_if

namespace lms {
namespace persistence_service {

InMemoryPersistenceService::InMemoryPersistenceService() = default;

// --- Author Operations ---
void InMemoryPersistenceService::saveAuthor(const std::shared_ptr<Author>& author) {
    if (!author) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    // Create a copy for storage if it's a different instance or to ensure it's managed here.
    // However, shared_ptr semantics usually mean we store the passed one.
    // For simplicity and consistency with unique_ptr cloning, one might clone here too,
    // but shared_ptr is designed for sharing, so direct storage is common.
    // Let's assume we are storing the shared_ptr as is.
    m_authors[author->getId()] = author;
}

std::optional<std::shared_ptr<Author>> InMemoryPersistenceService::loadAuthor(const EntityId& author_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_authors.find(author_id);
    if (it != m_authors.end()) {
        return it->second; // Returns a shared_ptr to the stored author
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<Author>> InMemoryPersistenceService::loadAllAuthors() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Author>> all_authors;
    all_authors.reserve(m_authors.size());
    for (const auto& pair : m_authors) {
        all_authors.push_back(pair.second);
    }
    return all_authors;
}

void InMemoryPersistenceService::deleteAuthor(const EntityId& author_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_authors.erase(author_id);
}

// --- Library Item Operations ---
void InMemoryPersistenceService::saveLibraryItem(const ILibraryItem* item) {
    if (!item) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_items[item->getId()] = item->clone(); // Store a clone
}

std::optional<std::unique_ptr<ILibraryItem>> InMemoryPersistenceService::loadLibraryItem(const EntityId& item_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_items.find(item_id);
    if (it != m_items.end() && it->second) {
        return it->second->clone(); // Return a clone of the stored item
    }
    return std::nullopt;
}

std::vector<std::unique_ptr<ILibraryItem>> InMemoryPersistenceService::loadAllLibraryItems() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::unique_ptr<ILibraryItem>> all_items;
    all_items.reserve(m_items.size());
    for (const auto& pair : m_items) {
        if (pair.second) {
            all_items.push_back(pair.second->clone());
        }
    }
    return all_items;
}

void InMemoryPersistenceService::deleteLibraryItem(const EntityId& item_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_items.erase(item_id);
}

// --- User Operations ---
void InMemoryPersistenceService::saveUser(const User& user) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // m_users[user.getUserId()] = user; // Stores a copy
    auto it = m_users.find(user.getUserId());
    if (it != m_users.end()) {
        it->second = user; // Update existing user
    } else {
        m_users.emplace(user.getUserId(), user); // Insert new user
    }
}

std::optional<User> InMemoryPersistenceService::loadUser(const EntityId& user_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_users.find(user_id);
    if (it != m_users.end()) {
        return it->second; // Returns a copy
    }
    return std::nullopt;
}

std::vector<User> InMemoryPersistenceService::loadAllUsers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<User> all_users;
    all_users.reserve(m_users.size());
    for (const auto& pair : m_users) {
        all_users.push_back(pair.second);
    }
    return all_users;
}

void InMemoryPersistenceService::deleteUser(const EntityId& user_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_users.erase(user_id);
}

// --- Loan Record Operations ---
void InMemoryPersistenceService::saveLoanRecord(const LoanRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // m_loan_records[record.getRecordId()] = record;
    // error: no matching function for call to ‘lms::domain_core::LoanRecord::LoanRecord()’

    // Fix
    auto it = m_loan_records.find(record.getRecordId());
    if (it != m_loan_records.end()) {
        it->second = record; // Update existing record
    } else {
        m_loan_records.emplace(record.getRecordId(), record); // Insert new record
    }
}

std::optional<LoanRecord> InMemoryPersistenceService::loadLoanRecord(const EntityId& record_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loan_records.find(record_id);
    if (it != m_loan_records.end()) {
        return it->second; // Returns a copy
    }
    return std::nullopt;
}

std::vector<LoanRecord> InMemoryPersistenceService::loadLoanRecordsByUserId(const EntityId& user_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LoanRecord> user_records;
    for (const auto& pair : m_loan_records) {
        if (pair.second.getUserId() == user_id) {
            user_records.push_back(pair.second);
        }
    }
    return user_records;
}

std::vector<LoanRecord> InMemoryPersistenceService::loadLoanRecordsByItemId(const EntityId& item_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LoanRecord> item_records;
    for (const auto& pair : m_loan_records) {
        if (pair.second.getItemId() == item_id) {
            item_records.push_back(pair.second);
        }
    }
    return item_records;
}

std::vector<LoanRecord> InMemoryPersistenceService::loadAllLoanRecords() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LoanRecord> all_records;
    all_records.reserve(m_loan_records.size());
    for (const auto& pair : m_loan_records) {
        all_records.push_back(pair.second);
    }
    return all_records;
}

void InMemoryPersistenceService::deleteLoanRecord(const EntityId& record_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loan_records.erase(record_id);
}

void InMemoryPersistenceService::updateLoanRecord(const LoanRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loan_records.find(record.getRecordId());
    if (it != m_loan_records.end()) {
        it->second = record; // Replace with the new record (copy)
    } else {
        // Or throw an exception if record to update not found, or save it as new
        // For now, let's just save it if it doesn't exist (upsert behavior)
        m_loan_records.emplace(record.getRecordId(), record);
        // A stricter update might be:
        // throw NotFoundException("LoanRecord with ID " + record.getRecordId() + " not found for update.");
    }
}

} // namespace persistence_service
} // namespace lms