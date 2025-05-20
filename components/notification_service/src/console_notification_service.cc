#include "notification_service/console_notification_service.h"
// No specific domain_core includes needed for this simple implementation if EntityId is from
// types.h

namespace lms {
namespace notification_service {

void ConsoleNotificationService::sendNotification(const EntityId& user_id,
                                                  const std::string& message) {
  // Basic validation
  if (user_id.empty()) {
    std::cerr << "[ConsoleNotificationService ERROR] User ID cannot be empty." << std::endl;
    // Or throw an exception, e.g., domain_core::InvalidArgumentException
    // throw domain_core::InvalidArgumentException("User ID cannot be empty for notification.");
    return;
  }
  if (message.empty()) {
    std::cerr
        << "[ConsoleNotificationService ERROR] Notification message cannot be empty for user '"
        << user_id << "'." << std::endl;
    // throw domain_core::InvalidArgumentException("Notification message cannot be empty.");
    return;
  }

  // Simple console output
  std::cout << "[NOTIFICATION to User '" << user_id << "']: " << message << std::endl;
}

}  // namespace notification_service
}  // namespace lms