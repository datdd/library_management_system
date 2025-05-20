#ifndef LMS_DOMAIN_CORE_I_LIBRARY_ITEM_H
#define LMS_DOMAIN_CORE_I_LIBRARY_ITEM_H

#include <memory>  // For shared_ptr to Author
#include <string>
#include "types.h"


namespace lms {
namespace domain_core {

class Author;  // Forward declaration

enum class AvailabilityStatus {
  AVAILABLE,
  BORROWED,
  RESERVED,    // Future enhancement
  MAINTENANCE  // Future enhancement
};

// Interface for any item that can be part of the library
class ILibraryItem {
public:
  virtual ~ILibraryItem() = default;

  virtual const EntityId& getId() const = 0;
  virtual const std::string& getTitle() const = 0;
  virtual void setTitle(std::string title) = 0;

  virtual AvailabilityStatus getAvailabilityStatus() const = 0;
  virtual void setAvailabilityStatus(AvailabilityStatus status) = 0;

  // Common method to get author, even if some items might not have one (e.g.
  // magazine) For items without a specific author, this could return nullptr or
  // a "generic" author. The document specifies Book having an Author.
  virtual std::shared_ptr<Author> getAuthor() const = 0;
  virtual void setAuthor(std::shared_ptr<Author> author) = 0;

  // Could add more common properties like publication year here,
  // or leave them to concrete types if they vary too much.
  // Document has publication year in Book.
  virtual int getPublicationYear() const = 0;
  virtual void setPublicationYear(int year) = 0;
  virtual std::unique_ptr<ILibraryItem> clone() const = 0;
};

}  // namespace domain_core
}  // namespace lms

#endif  // LMS_DOMAIN_CORE_I_LIBRARY_ITEM_H