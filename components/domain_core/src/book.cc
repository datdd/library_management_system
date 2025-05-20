#include "domain_core/book.h"

#include "domain_core/types.h"  // For InvalidArgumentException

namespace lms {
namespace domain_core {

Book::Book(EntityId id,
           std::string title,
           std::shared_ptr<Author> author,
           std::string isbn,
           int publication_year,
           AvailabilityStatus status)
    : m_id(std::move(id)),
      m_title(std::move(title)),
      m_author(std::move(author)),
      m_isbn(std::move(isbn)),
      m_publication_year(publication_year),
      m_availability_status(status) {
  if (m_id.empty()) {
    throw InvalidArgumentException("Book ID cannot be empty.");
  }
  if (m_title.empty()) {
    throw InvalidArgumentException("Book title cannot be empty.");
  }
  if (!m_author) {
    throw InvalidArgumentException("Book author cannot be null.");
  }
  if (m_isbn.empty()) {
    // ISBN might be optional for some books, but for now let's enforce it
    throw InvalidArgumentException("Book ISBN cannot be empty.");
  }
  if (m_publication_year <= 0) {  // Basic validation
    throw InvalidArgumentException("Publication year must be positive.");
  }
}

const EntityId& Book::getId() const {
  return m_id;
}

const std::string& Book::getTitle() const {
  return m_title;
}

void Book::setTitle(std::string title) {
  if (title.empty()) {
    throw InvalidArgumentException("Book title cannot be empty.");
  }
  m_title = std::move(title);
}

AvailabilityStatus Book::getAvailabilityStatus() const {
  return m_availability_status;
}

void Book::setAvailabilityStatus(AvailabilityStatus status) {
  m_availability_status = status;
}

std::shared_ptr<Author> Book::getAuthor() const {
  return m_author;
}

void Book::setAuthor(std::shared_ptr<Author> author) {
  if (!author) {
    throw InvalidArgumentException("Book author cannot be null.");
  }
  m_author = std::move(author);
}

int Book::getPublicationYear() const {
  return m_publication_year;
}

void Book::setPublicationYear(int year) {
  if (year <= 0) {
    throw InvalidArgumentException("Publication year must be positive.");
  }
  m_publication_year = year;
}

const std::string& Book::getIsbn() const {
  return m_isbn;
}

void Book::setIsbn(std::string isbn) {
  if (isbn.empty()) {
    throw InvalidArgumentException("Book ISBN cannot be empty.");
  }
  m_isbn = std::move(isbn);
}

std::unique_ptr<ILibraryItem> Book::clone() const {
  // Create a new Book by copying current state.
  // Note: Author is a shared_ptr, so it's shared, not deep-cloned here.
  // This is consistent with the design (authors shared among books).
  return std::make_unique<Book>(m_id, m_title, m_author, m_isbn, m_publication_year,
                                m_availability_status);
}

bool Book::operator==(const Book& other) const {
  bool authors_equal = false;
  if (m_author && other.m_author) {
    authors_equal = (*m_author == *other.m_author);
  } else if (!m_author && !other.m_author) {
    authors_equal = true;  // Both null
  }

  return m_id == other.m_id && m_title == other.m_title && authors_equal &&
         m_isbn == other.m_isbn && m_publication_year == other.m_publication_year &&
         m_availability_status == other.m_availability_status;
}

bool Book::operator!=(const Book& other) const {
  return !(*this == other);
}

}  // namespace domain_core
}  // namespace lms