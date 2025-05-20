#ifndef LMS_TESTS_MOCKS_MOCK_CATALOG_SERVICE_H
#define LMS_TESTS_MOCKS_MOCK_CATALOG_SERVICE_H

#include "catalog_service/i_catalog_service.h"
#include "gmock/gmock.h"

namespace lms {
namespace testing {
using namespace catalog_service;
class MockCatalogService : public ICatalogService {
public:
  MOCK_METHOD(void,
              addBook,
              (const EntityId&,
               const std::string&,
               const EntityId&,
               const std::string&,
               const std::string&,
               int),
              (override));
  MOCK_METHOD(bool, removeItem, (const EntityId& item_id), (override));
  MOCK_METHOD(std::optional<std::unique_ptr<ILibraryItem>>,
              findItemById,
              (const EntityId& item_id),
              (const, override));
  MOCK_METHOD(std::vector<std::unique_ptr<ILibraryItem>>,
              findItemsByTitle,
              (const std::string& title),
              (const, override));
  MOCK_METHOD(std::vector<std::unique_ptr<ILibraryItem>>,
              findItemsByAuthor,
              (const EntityId& author_id),
              (const, override));
  MOCK_METHOD(std::vector<std::unique_ptr<ILibraryItem>>, getAllItems, (), (const, override));
  MOCK_METHOD(void,
              updateItemStatus,
              (const EntityId& item_id, AvailabilityStatus new_status),
              (override));
};
}  // namespace testing
}  // namespace lms
#endif