#ifndef LMS_DOMAIN_CORE_BOOK_H
#define LMS_DOMAIN_CORE_BOOK_H

#include <memory>
#include <string>
#include "author.h"  // Include full definition for shared_ptr
#include "i_library_item.h"


namespace lms {
namespace domain_core {

class Book : public ILibraryItem {
public:
  Book(EntityId id,
       std::string title,
       std::shared_ptr<Author> author,
       std::string isbn,
       int publication_year,
       AvailabilityStatus status = AvailabilityStatus::AVAILABLE);

  // ILibraryItem interface implementation
  const EntityId& getId() const override;
  const std::string& getTitle() const override;
  void setTitle(std::string title) override;

  AvailabilityStatus getAvailabilityStatus() const override;
  void setAvailabilityStatus(AvailabilityStatus status) override;

  std::shared_ptr<Author> getAuthor() const override;
  void setAuthor(std::shared_ptr<Author> author) override;

  int getPublicationYear() const override;
  void setPublicationYear(int year) override;

  // Book-specific methods
  const std::string& getIsbn() const;
  void setIsbn(std::string isbn);

  // Clone method for polymorphic behavior
  std::unique_ptr<ILibraryItem> clone() const override;

  // Comparison operators
  bool operator==(const Book& other) const;
  bool operator!=(const Book& other) const;

private:
  EntityId m_id;
  std::string m_title;
  std::shared_ptr<Author> m_author;
  std::string m_isbn;
  int m_publication_year;
  AvailabilityStatus m_availability_status;
};

}  // namespace domain_core
}  // namespace lms

#endif  // LMS_DOMAIN_CORE_BOOK_H