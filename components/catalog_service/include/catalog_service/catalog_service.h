#ifndef LMS_CATALOG_SERVICE_CATALOG_SERVICE_H
#define LMS_CATALOG_SERVICE_CATALOG_SERVICE_H

#include <map>     // For internal cache/management of items if not relying solely on persistence
#include <memory>  // For std::shared_ptr (for persistence_service)
#include <mutex>   // For thread safety
#include "i_catalog_service.h"
#include "persistence_service/i_persistence_service.h"

namespace lms {
namespace catalog_service {

class CatalogService : public ICatalogService {
public:
  explicit CatalogService(
      std::shared_ptr<persistence_service::IPersistenceService> persistence_service);
  ~CatalogService() override = default;

  void addBook(const EntityId& item_id,
               const std::string& title,
               const EntityId& author_id,
               const std::string& author_name,
               const std::string& isbn,
               int publication_year) override;

  bool removeItem(const EntityId& item_id) override;
  std::optional<std::unique_ptr<ILibraryItem>> findItemById(const EntityId& item_id) const override;
  std::vector<std::unique_ptr<ILibraryItem>> findItemsByTitle(
      const std::string& title) const override;
  std::vector<std::unique_ptr<ILibraryItem>> findItemsByAuthor(
      const EntityId& author_id) const override;
  std::vector<std::unique_ptr<ILibraryItem>> getAllItems() const override;
  void updateItemStatus(const EntityId& item_id, AvailabilityStatus new_status) override;

private:
  std::shared_ptr<persistence_service::IPersistenceService> m_persistence_service;

  // Helper method to get or create an author
  std::shared_ptr<Author> getOrCreateAuthor(const EntityId& author_id,
                                            const std::string& author_name);

  // The document implies CatalogService uses a map of unique_ptr<ILibraryItem> internally.
  // This means the CatalogService itself might hold items in memory (as a cache or primary store)
  // and syncs with IPersistenceService.
  // Let's assume it primarily relies on IPersistenceService for loading,
  // but manages item instances it gives out or works with.
  // If it were a cache:
  // mutable std::map<EntityId, std::unique_ptr<ILibraryItem>> m_item_cache;
  // mutable std::mutex m_cache_mutex;
  // For simplicity, we'll have it fetch from persistence each time for "find" methods,
  // and save/delete through persistence. Update status will load, modify, save.
};

}  // namespace catalog_service
}  // namespace lms

#endif  // LMS_CATALOG_SERVICE_CATALOG_SERVICE_H