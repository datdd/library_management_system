#ifndef LMS_DOMAIN_CORE_USER_H
#define LMS_DOMAIN_CORE_USER_H

#include <string>
#include "types.h"

namespace lms {
namespace domain_core {

class User {
public:
  User(EntityId user_id, std::string name);

  const EntityId& getUserId() const;
  const std::string& getName() const;
  void setName(std::string name);

  bool operator==(const User& other) const;
  bool operator!=(const User& other) const;

private:
  EntityId m_user_id;
  std::string m_name;
};

}  // namespace domain_core
}  // namespace lms

#endif  // LMS_DOMAIN_CORE_USER_H