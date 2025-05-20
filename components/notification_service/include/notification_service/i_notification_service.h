#ifndef LMS_NOTIFICATION_SERVICE_I_NOTIFICATION_SERVICE_H
#define LMS_NOTIFICATION_SERVICE_I_NOTIFICATION_SERVICE_H

#include <string>
#include "domain_core/types.h"  // For EntityId (user_id)

namespace lms {
namespace notification_service {

// Using domain_core::EntityId
using domain_core::EntityId;

class INotificationService {
public:
  virtual ~INotificationService() = default;

  // Sends a notification message to a specific user.
  // How the user is identified or how the message is delivered
  // is up to the concrete implementation.
  virtual void sendNotification(const EntityId& user_id, const std::string& message) = 0;
};

}  // namespace notification_service
}  // namespace lms

#endif  // LMS_NOTIFICATION_SERVICE_I_NOTIFICATION_SERVICE_H