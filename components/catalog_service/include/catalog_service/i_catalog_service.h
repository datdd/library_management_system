#ifndef LMS_CATALOG_SERVICE_I_CATALOG_SERVICE_H
#define LMS_CATALOG_SERVICE_I_CATALOG_SERVICE_H

#include "domain_core/author.h"  // For creating/managing authors potentially
#include "domain_core/i_library_item.h"
#include "domain_core/types.h"  // For EntityId, std::optional, exceptions

#include <memory>  // For std::unique_ptr, std::shared_ptr
#include <optional>
#include <string>
#include <vector>

namespace lms {
namespace catalog_service {

using namespace domain_core;  // Make domain types easily accessible

class ICatalogService {
public:
  virtual ~ICatalogService() = default;

  // Adds a new book to the catalog.
  // Manages Author creation/retrieval implicitly or explicitly.
  // For simplicity, let's assume author details are passed, and the service handles
  // ensuring the author exists or creates/retrieves it.
  virtual void addBook(const EntityId& item_id,
                       const std::string& title,
                       const EntityId& author_id,       // ID of an existing or new author
                       const std::string& author_name,  // Name, used if author_id is new
                       const std::string& isbn,
                       int publication_year) = 0;

  // Adds a generic library item (could be used if we have a factory later)
  // For now, addBook is more specific as per document scope.
  // virtual void addItem(std::unique_ptr<ILibraryItem> item) = 0;

  // Removes an item from the catalog by its ID.
  // Returns true if an item was removed, false if no item with that ID was found.
  virtual bool removeItem(const EntityId& item_id) = 0;

  // Finds an item by its ID.
  // Returns std::nullopt if no item is found.
  virtual std::optional<std::unique_ptr<ILibraryItem>> findItemById(
      const EntityId& item_id) const = 0;

  // Finds items by title (exact match for now).
  virtual std::vector<std::unique_ptr<ILibraryItem>> findItemsByTitle(
      const std::string& title) const = 0;

  // Finds items by author ID.
  virtual std::vector<std::unique_ptr<ILibraryItem>> findItemsByAuthor(
      const EntityId& author_id) const = 0;

  // Retrieves all items in the catalog.
  virtual std::vector<std::unique_ptr<ILibraryItem>> getAllItems() const = 0;

  // Updates an item's availability status.
  // Throws NotFoundException if item_id does not exist.
  virtual void updateItemStatus(const EntityId& item_id, AvailabilityStatus new_status) = 0;

  // (Future: updateItemDetails, etc.)
};

}  // namespace catalog_service
}  // namespace lms

#endif  // LMS_CATALOG_SERVICE_I_CATALOG_SERVICE_H