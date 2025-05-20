#include <algorithm>  // For std::transform
#include <iostream>
#include <limits>  // For std::numeric_limits
#include <memory>  // For std::make_shared
#include <sstream>
#include <string>
#include <vector>

// Domain Core (for exceptions, enums, etc. that might be caught or used directly)
#include "domain_core/i_library_item.h"  // For AvailabilityStatus enum to string
#include "domain_core/types.h"
#include "domain_core/book.h"  // For Book class

// Service Interfaces (though we'll be creating concrete ones)
// Not strictly needed to include interfaces if we only use concrete types here,
// but good for reference.

// Concrete Service Implementations
#include "catalog_service/catalog_service.h"
#include "loan_service/loan_service.h"
#include "notification_service/console_notification_service.h"
#include "persistence_service/in_memory_persistence_service.h"
#include "user_service/user_service.h"
#include "utils/date_time_utils.h"

// Helper function to split string by delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

// Helper to convert AvailabilityStatus to string
std::string availabilityStatusToString(lms::domain_core::AvailabilityStatus status) {
  using AS = lms::domain_core::AvailabilityStatus;
  switch (status) {
    case AS::AVAILABLE:
      return "Available";
    case AS::BORROWED:
      return "Borrowed";
    case AS::RESERVED:
      return "Reserved";
    case AS::MAINTENANCE:
      return "Maintenance";
    default:
      return "Unknown";
  }
}

void printHelp() {
  std::cout << "\nLibrary Management System CLI" << std::endl;
  std::cout << "Available commands:" << std::endl;
  std::cout << "  addUser <user_id> <name>" << std::endl;
  std::cout << "  findUser <user_id>" << std::endl;
  std::cout << "  listUsers" << std::endl;
  std::cout << "  addBook <item_id> \"<title>\" <author_id> \"<author_name>\" <isbn> <year>"
            << std::endl;
  std::cout << "  findItem <item_id>" << std::endl;
  std::cout << "  listItems" << std::endl;
  std::cout << "  borrow <user_id> <item_id>" << std::endl;
  std::cout << "  return <user_id> <item_id>" << std::endl;
  std::cout << "  userLoans <user_id>" << std::endl;
  std::cout << "  itemHistory <item_id>" << std::endl;
  std::cout << "  checkOverdue" << std::endl;
  std::cout << "  help" << std::endl;
  std::cout << "  exit" << std::endl;
  std::cout << "Note: Use quotes for multi-word titles and names." << std::endl;
}

// Function to read a line, handling potential quotes for multi-word arguments
std::vector<std::string> readCommandArgs(const std::string& line) {
  std::vector<std::string> args;
  std::string current_arg;
  bool in_quotes = false;

  for (char ch : line) {
    if (ch == '"') {
      in_quotes = !in_quotes;
      if (!in_quotes && !current_arg.empty()) {  // End of quoted argument
        args.push_back(current_arg);
        current_arg.clear();
      }
    } else if (ch == ' ' && !in_quotes) {
      if (!current_arg.empty()) {
        args.push_back(current_arg);
        current_arg.clear();
      }
    } else {
      current_arg += ch;
    }
  }
  if (!current_arg.empty()) {  // Add last argument
    args.push_back(current_arg);
  }
  return args;
}

int main() {
  // 1. Instantiate services
  auto persistence_service =
      std::make_shared<lms::persistence_service::InMemoryPersistenceService>();
  auto date_time_utils = std::make_shared<lms::utils::DateTimeUtils>();

  auto user_service = std::make_shared<lms::user_service::UserService>(persistence_service);
  auto catalog_service =
      std::make_shared<lms::catalog_service::CatalogService>(persistence_service);
  auto notification_service =
      std::make_shared<lms::notification_service::ConsoleNotificationService>();

  const int default_loan_duration_days = 14;  // Can be configurable
  auto loan_service = std::make_shared<lms::loan_service::LoanService>(
      catalog_service, user_service, persistence_service, notification_service, date_time_utils,
      default_loan_duration_days);

  std::string line;
  printHelp();

  while (true) {
    std::cout << "\nlms> ";
    if (!std::getline(std::cin, line)) {
      break;  // EOF or error
    }

    if (line.empty()) {
      continue;
    }

    std::vector<std::string> args = readCommandArgs(line);
    if (args.empty()) {
      continue;
    }

    std::string command = args[0];
    std::transform(command.begin(), command.end(), command.begin(),
                   ::tolower);  // Make command case-insensitive

    try {
      if (command == "exit") {
        std::cout << "Exiting LMS." << std::endl;
        break;
      } else if (command == "help") {
        printHelp();
      } else if (command == "adduser" && args.size() == 3) {
        user_service->addUser(args[1], args[2]);
        std::cout << "User '" << args[2] << "' with ID '" << args[1] << "' added." << std::endl;
      } else if (command == "finduser" && args.size() == 2) {
        auto user_opt = user_service->findUserById(args[1]);
        if (user_opt) {
          std::cout << "User found: ID=" << user_opt->getUserId()
                    << ", Name=" << user_opt->getName() << std::endl;
        } else {
          std::cout << "User with ID '" << args[1] << "' not found." << std::endl;
        }
      } else if (command == "listusers") {
        auto users = user_service->getAllUsers();
        if (users.empty()) {
          std::cout << "No users in the system." << std::endl;
        } else {
          std::cout << "Users:" << std::endl;
          for (const auto& user : users) {
            std::cout << "  ID: " << user.getUserId() << ", Name: " << user.getName() << std::endl;
          }
        }
      } else if (command == "addbook" && args.size() == 7) {
        // addBook <item_id> "<title>" <author_id> "<author_name>" <isbn> <year>
        int year = 0;
        try {
          year = std::stoi(args[6]);
        } catch (const std::invalid_argument& ia) {
          std::cerr << "Error: Invalid year format for addBook." << std::endl;
          continue;
        } catch (const std::out_of_range& oor) {
          std::cerr << "Error: Year out of range for addBook." << std::endl;
          continue;
        }
        catalog_service->addBook(args[1], args[2], args[3], args[4], args[5], year);
        std::cout << "Book '" << args[2] << "' added with ID '" << args[1] << "'." << std::endl;
      } else if (command == "finditem" && args.size() == 2) {
        auto item_opt = catalog_service->findItemById(args[1]);
        if (item_opt.has_value() && item_opt.value()) {
          const auto& item = item_opt.value();
          std::cout << "Item found: ID=" << item->getId() << ", Title=" << item->getTitle()
                    << ", Status=" << availabilityStatusToString(item->getAvailabilityStatus());
          if (item->getAuthor()) {
            std::cout << ", Author=" << item->getAuthor()->getName();
          }
          // If it's a Book, print ISBN and Year
          const lms::domain_core::Book* book =
              dynamic_cast<const lms::domain_core::Book*>(item.get());
          if (book) {
            std::cout << ", ISBN=" << book->getIsbn() << ", Year=" << book->getPublicationYear();
          }
          std::cout << std::endl;
        } else {
          std::cout << "Item with ID '" << args[1] << "' not found." << std::endl;
        }
      } else if (command == "listitems") {
        auto items = catalog_service->getAllItems();
        if (items.empty()) {
          std::cout << "No items in the catalog." << std::endl;
        } else {
          std::cout << "Catalog Items:" << std::endl;
          for (const auto& item_ptr : items) {
            if (item_ptr) {
              std::cout << "  ID: " << item_ptr->getId() << ", Title: " << item_ptr->getTitle()
                        << ", Status: "
                        << availabilityStatusToString(item_ptr->getAvailabilityStatus());
              if (item_ptr->getAuthor()) {
                std::cout << ", Author: " << item_ptr->getAuthor()->getName();
              }
              if (const auto* book = dynamic_cast<const lms::domain_core::Book*>(item_ptr.get())) {
                std::cout << ", ISBN=" << book->getIsbn()
                          << ", Year=" << book->getPublicationYear();
              }
              std::cout << std::endl;
            }
          }
        }
      } else if (command == "borrow" && args.size() == 3) {
        lms::domain_core::LoanRecord loan = loan_service->borrowItem(args[1], args[2]);
        std::cout << "Item '" << args[2] << "' borrowed by user '" << args[1] << "'." << std::endl;
        std::cout << "  Loan ID: " << loan.getRecordId()
                  << ", Due Date: " << date_time_utils->formatDate(loan.getDueDate()) << std::endl;
      } else if (command == "return" && args.size() == 3) {
        loan_service->returnItem(args[1], args[2]);
        std::cout << "Item '" << args[2] << "' returned by user '" << args[1] << "'." << std::endl;
      } else if (command == "userloans" && args.size() == 2) {
        auto loans = loan_service->getActiveLoansForUser(args[1]);
        if (loans.empty()) {
          std::cout << "No active loans for user '" << args[1] << "'." << std::endl;
        } else {
          std::cout << "Active loans for user '" << args[1] << "':" << std::endl;
          for (const auto& loan : loans) {
            std::cout << "  Loan ID: " << loan.getRecordId() << ", Item ID: " << loan.getItemId()
                      << ", Loan Date: " << date_time_utils->formatDate(loan.getLoanDate())
                      << ", Due Date: " << date_time_utils->formatDate(loan.getDueDate())
                      << std::endl;
          }
        }
      } else if (command == "itemhistory" && args.size() == 2) {
        auto loans = loan_service->getLoanHistoryForItem(args[1]);
        if (loans.empty()) {
          std::cout << "No loan history for item '" << args[1] << "'." << std::endl;
        } else {
          std::cout << "Loan history for item '" << args[1] << "':" << std::endl;
          for (const auto& loan : loans) {
            std::cout << "  Loan ID: " << loan.getRecordId() << ", User ID: " << loan.getUserId()
                      << ", Loan Date: " << date_time_utils->formatDate(loan.getLoanDate())
                      << ", Due Date: " << date_time_utils->formatDate(loan.getDueDate());
            if (loan.getReturnDate().has_value()) {
              std::cout << ", Returned: "
                        << date_time_utils->formatDate(loan.getReturnDate().value());
            } else {
              std::cout << ", Status: Active";
            }
            std::cout << std::endl;
          }
        }
      } else if (command == "checkoverdue") {
        std::cout << "Checking for overdue items and sending notifications..." << std::endl;
        loan_service->processOverdueItems();
        std::cout << "Overdue check complete. Check console for notifications." << std::endl;
      } else {
        std::cout << "Unknown command or incorrect arguments. Type 'help' for commands."
                  << std::endl;
      }
    } catch (const lms::domain_core::LmsException& e) {
      std::cerr << "Error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
    }
  }

  return 0;
}