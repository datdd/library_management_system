#ifndef LMS_DOMAIN_CORE_AUTHOR_H
#define LMS_DOMAIN_CORE_AUTHOR_H

#include <memory>  // For std::enable_shared_from_this if needed, but not strictly required by doc
#include <string>
#include "types.h"

namespace lms {
namespace domain_core {

class Author {
public:
  Author(EntityId id, std::string name);

  const EntityId& getId() const;
  const std::string& getName() const;
  void setName(std::string name);

  bool operator==(const Author& other) const;
  bool operator!=(const Author& other) const;

private:
  EntityId m_id;
  std::string m_name;
};

}  // namespace domain_core
}  // namespace lms

#endif  // LMS_DOMAIN_CORE_AUTHOR_H