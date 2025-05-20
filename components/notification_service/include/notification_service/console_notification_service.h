#ifndef LMS_NOTIFICATION_SERVICE_CONSOLE_NOTIFICATION_SERVICE_H
#define LMS_NOTIFICATION_SERVICE_CONSOLE_NOTIFICATION_SERVICE_H

#include <iostream>  // For std::cout
#include "i_notification_service.h"

namespace lms {
namespace notification_service {

class ConsoleNotificationService : public INotificationService {
public:
  ConsoleNotificationService() = default;
  ~ConsoleNotificationService() override = default;

  void sendNotification(const EntityId& user_id, const std::string& message) override;

private:
  // In a real scenario, might have a prefix or formatting options
  // std::string m_log_prefix = "[CONSOLE_NOTIFICATION]";
};

}  // namespace notification_service
}  // namespace lms

#endif  // LMS_NOTIFICATION_SERVICE_CONSOLE_NOTIFICATION_SERVICE_H