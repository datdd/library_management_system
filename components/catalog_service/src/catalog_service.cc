#include "catalog_service/catalog_service.h"
#include "domain_core/book.h"   // For creating Book instances
#include "domain_core/types.h"  // For exceptions

#include <algorithm>  // For std::remove_if if managing an in-memory list

namespace lms {
namespace catalog_service {

CatalogService::CatalogService(
    std::shared_ptr<persistence_service::IPersistenceService> persistence_service)
    : m_persistence_service(std::move(persistence_service)) {
  if (!m_persistence_service) {
    throw std::invalid_argument("Persistence service cannot be null for CatalogService.");
  }
}

std::shared_ptr<Author> CatalogService::getOrCreateAuthor(const EntityId& author_id,
                                                          const std::string& author_name) {
  // Try to load the author from persistence
  auto author_opt = m_persistence_service->loadAuthor(author_id);
  if (author_opt.has_value()) {
    return author_opt.value();
  } else {
    // Author not found, create and save a new one
    if (author_id.empty() || author_name.empty()) {
      throw InvalidArgumentException("Author ID and name must be provided to create a new author.");
    }
    auto new_author = std::make_shared<Author>(author_id, author_name);
    m_persistence_service->saveAuthor(new_author);
    return new_author;
  }
}

void CatalogService::addBook(const EntityId& item_id,
                             const std::string& title,
                             const EntityId& author_id,
                             const std::string& author_name,
                             const std::string& isbn,
                             int publication_year) {
  if (item_id.empty() || title.empty() || isbn.empty() || publication_year <= 0) {
    throw InvalidArgumentException("Invalid parameters for adding a book.");
  }

  // Check if item already exists
  if (m_persistence_service->loadLibraryItem(item_id).has_value()) {
    throw OperationFailedException("Library item with ID '" + item_id + "' already exists.");
  }

  std::shared_ptr<Author> author = getOrCreateAuthor(author_id, author_name);
  if (!author) {  // Should not happen if getOrCreateAuthor throws on failure
    throw OperationFailedException("Could not get or create author for the book.");
  }

  auto new_book = std::make_unique<Book>(item_id, title, author, isbn, publication_year);
  m_persistence_service->saveLibraryItem(new_book.get());  // Persistence service clones it
}

bool CatalogService::removeItem(const EntityId& item_id) {
  if (item_id.empty()) {
    throw InvalidArgumentException("Item ID cannot be empty for removeItem.");
  }
  // Check if item exists before attempting delete for accurate return
  if (!m_persistence_service->loadLibraryItem(item_id).has_value()) {
    return false;
  }
  m_persistence_service->deleteLibraryItem(item_id);
  return true;  // Assume delete succeeds if it doesn't throw
}

std::optional<std::unique_ptr<ILibraryItem>> CatalogService::findItemById(
    const EntityId& item_id) const {
  if (item_id.empty()) {
    throw InvalidArgumentException("Item ID cannot be empty for findItemById.");
  }
  return m_persistence_service->loadLibraryItem(item_id);
}

std::vector<std::unique_ptr<ILibraryItem>> CatalogService::findItemsByTitle(
    const std::string& title) const {
  if (title.empty()) {
    throw InvalidArgumentException("Title cannot be empty for findItemsByTitle.");
  }
  std::vector<std::unique_ptr<ILibraryItem>> all_items =
      m_persistence_service->loadAllLibraryItems();
  std::vector<std::unique_ptr<ILibraryItem>> matching_items;

  for (auto& item : all_items) {  // item is unique_ptr
    if (item && item->getTitle() == title) {
      // Need to move the item if we are returning it, or clone if all_items is to be preserved.
      // Since loadAllLibraryItems returns new clones, we can move from all_items.
      matching_items.push_back(std::move(item));
    }
  }
  // Clean up nullptrs from all_items if any were moved.
  all_items.erase(
      std::remove_if(all_items.begin(), all_items.end(), [](const auto& ptr) { return !ptr; }),
      all_items.end());

  return matching_items;
}

std::vector<std::unique_ptr<ILibraryItem>> CatalogService::findItemsByAuthor(
    const EntityId& author_id) const {
  if (author_id.empty()) {
    throw InvalidArgumentException("Author ID cannot be empty for findItemsByAuthor.");
  }
  std::vector<std::unique_ptr<ILibraryItem>> all_items =
      m_persistence_service->loadAllLibraryItems();
  std::vector<std::unique_ptr<ILibraryItem>> matching_items;

  for (auto& item : all_items) {
    if (item && item->getAuthor() && item->getAuthor()->getId() == author_id) {
      matching_items.push_back(std::move(item));
    }
  }
  all_items.erase(
      std::remove_if(all_items.begin(), all_items.end(), [](const auto& ptr) { return !ptr; }),
      all_items.end());
  return matching_items;
}

std::vector<std::unique_ptr<ILibraryItem>> CatalogService::getAllItems() const {
  return m_persistence_service->loadAllLibraryItems();
}

void CatalogService::updateItemStatus(const EntityId& item_id, AvailabilityStatus new_status) {
  if (item_id.empty()) {
    throw InvalidArgumentException("Item ID cannot be empty for updateItemStatus.");
  }

  auto item_opt = m_persistence_service->loadLibraryItem(item_id);
  if (!item_opt.has_value() || !item_opt.value()) {
    throw NotFoundException("Item with ID '" + item_id + "' not found for status update.");
  }

  std::unique_ptr<ILibraryItem> item_to_update = std::move(item_opt.value());
  item_to_update->setAvailabilityStatus(new_status);
  m_persistence_service->saveLibraryItem(
      item_to_update.get());  // saveLibraryItem will overwrite/update
}

}  // namespace catalog_service
}  // namespace lms