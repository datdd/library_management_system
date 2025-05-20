#ifndef LMS_TESTS_USER_SERVICE_MOCK_PERSISTENCE_SERVICE_H
#define LMS_TESTS_USER_SERVICE_MOCK_PERSISTENCE_SERVICE_H

#include "gmock/gmock.h"
#include "persistence_service/i_persistence_service.h"

// Forward declare domain types needed by IPersistenceService if not fully included by
// i_persistence_service.h However, i_persistence_service.h should already include them.

namespace lms {
namespace testing {  // A common namespace for test utilities/mocks

// Using lms::persistence_service::EntityId, etc.
using namespace domain_core;
using namespace persistence_service;

class MockPersistenceService : public IPersistenceService {
public:
  MOCK_METHOD(void, saveAuthor, (const std::shared_ptr<Author>& author), (override));
  MOCK_METHOD(std::optional<std::shared_ptr<Author>>,
              loadAuthor,
              (const EntityId& author_id),
              (override));
  MOCK_METHOD(std::vector<std::shared_ptr<Author>>, loadAllAuthors, (), (override));
  MOCK_METHOD(void, deleteAuthor, (const EntityId& author_id), (override));

  // For ILibraryItem, the original takes ILibraryItem*
  // GMock needs the type exactly.
  MOCK_METHOD(void, saveLibraryItem, (const ILibraryItem* item), (override));
  MOCK_METHOD(std::optional<std::unique_ptr<ILibraryItem>>,
              loadLibraryItem,
              (const EntityId& item_id),
              (override));
  MOCK_METHOD(std::vector<std::unique_ptr<ILibraryItem>>, loadAllLibraryItems, (), (override));
  MOCK_METHOD(void, deleteLibraryItem, (const EntityId& item_id), (override));

  MOCK_METHOD(void, saveUser, (const User& user), (override));
  MOCK_METHOD(std::optional<User>, loadUser, (const EntityId& user_id), (override));
  MOCK_METHOD(std::vector<User>, loadAllUsers, (), (override));
  MOCK_METHOD(void, deleteUser, (const EntityId& user_id), (override));

  MOCK_METHOD(void, saveLoanRecord, (const LoanRecord& record), (override));
  MOCK_METHOD(std::optional<LoanRecord>, loadLoanRecord, (const EntityId& record_id), (override));
  MOCK_METHOD(std::vector<LoanRecord>,
              loadLoanRecordsByUserId,
              (const EntityId& user_id),
              (override));
  MOCK_METHOD(std::vector<LoanRecord>,
              loadLoanRecordsByItemId,
              (const EntityId& item_id),
              (override));
  MOCK_METHOD(std::vector<LoanRecord>, loadAllLoanRecords, (), (override));
  MOCK_METHOD(void, deleteLoanRecord, (const EntityId& record_id), (override));
  MOCK_METHOD(void, updateLoanRecord, (const LoanRecord& record), (override));
};

}  // namespace testing
}  // namespace lms

#endif  // LMS_TESTS_USER_SERVICE_MOCK_PERSISTENCE_SERVICE_H