#ifndef LMS_TESTS_MOCKS_MOCK_NOTIFICATION_SERVICE_H
#define LMS_TESTS_MOCKS_MOCK_NOTIFICATION_SERVICE_H

#include "gmock/gmock.h"
#include "notification_service/i_notification_service.h"

namespace lms {
namespace testing {
using namespace notification_service;
class MockNotificationService : public INotificationService {
public:
  MOCK_METHOD(void,
              sendNotification,
              (const EntityId& user_id, const std::string& message),
              (override));
};
}  // namespace testing
}  // namespace lms
#endif