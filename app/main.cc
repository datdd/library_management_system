#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// Domain Core
#include "domain_core/book.h"
#include "domain_core/i_library_item.h"
#include "domain_core/types.h"

// Service Interfaces & Implementations
#include "persistence_service/caching_file_persistence_service.h"
#include "persistence_service/file_persistence_service.h"
#include "persistence_service/i_persistence_service.h"
#include "persistence_service/in_memory_persistence_service.h"
#include "persistence_service/ms_sql_persistence_service.h"

#include "catalog_service/catalog_service.h"
#include "loan_service/loan_service.h"
#include "notification_service/console_notification_service.h"
#include "user_service/user_service.h"
#include "utils/date_time_utils.h"

// --- Forward Declarations for Application Structure ---
struct AppServices {
  std::shared_ptr<lms::utils::DateTimeUtils> date_time_utils;
  std::shared_ptr<lms::persistence_service::IPersistenceService> persistence_service_interface;
  std::shared_ptr<lms::persistence_service::CachingFilePersistenceService>
      caching_file_persistence_service;  // Might be null
  std::shared_ptr<lms::user_service::IUserService> user_service;
  std::shared_ptr<lms::catalog_service::ICatalogService> catalog_service;
  std::shared_ptr<lms::notification_service::INotificationService> notification_service;
  std::shared_ptr<lms::loan_service::ILoanService> loan_service;
};

bool initializeServices(AppServices& services);
void printHelp();
std::vector<std::string> readCommandArgs(
    const std::string& line);  // Assuming this is defined as before
std::string availabilityStatusToString(
    lms::domain_core::AvailabilityStatus status);  // Assuming defined
bool processCommand(const std::vector<std::string>& args, AppServices& services, bool& should_exit);
void runCliLoop(AppServices& services);

// --- Helper Functions ---

// (readCommandArgs remains the same as before)
std::vector<std::string> readCommandArgs(const std::string& line) {
  std::vector<std::string> args;
  std::string current_arg;
  bool in_quotes = false;

  for (char ch : line) {
    if (ch == '"') {
      in_quotes = !in_quotes;
      if (!in_quotes && !current_arg.empty()) {
        args.push_back(current_arg);
        current_arg.clear();
      } else if (in_quotes && current_arg.empty() && args.empty()) {
        // Start of quoted first argument, do nothing yet
      } else if (in_quotes && !current_arg.empty() && ch == '"') {
        // Handles "" as an empty string if needed by pushing current_arg
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
  if (!current_arg.empty()) {
    args.push_back(current_arg);
  }
  return args;
}

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
  std::cout << "---------------------------------" << std::endl;
  std::cout << "User Management:" << std::endl;
  std::cout << "  addUser <user_id> \"<full name>\"" << std::endl;
  std::cout << "  findUser <user_id>" << std::endl;
  std::cout << "  listUsers" << std::endl;
  std::cout << "Catalog Management:" << std::endl;
  std::cout << "  addBook <item_id> \"<title>\" <author_id> \"<author_name>\" <isbn> <year>"
            << std::endl;
  std::cout << "  findItem <item_id>" << std::endl;
  std::cout << "  listItems" << std::endl;
  std::cout << "Loan Management:" << std::endl;
  std::cout << "  borrow <user_id> <item_id>" << std::endl;
  std::cout << "  return <user_id> <item_id>" << std::endl;
  std::cout << "  userLoans <user_id>          (Show active loans for user)" << std::endl;
  std::cout << "  itemHistory <item_id>        (Show all loans for item)" << std::endl;
  std::cout << "  checkOverdue" << std::endl;
  std::cout << "Persistence (if CachingFilePersistence is active):" << std::endl;
  std::cout << "  saveAll                      (Save in-memory data to files)" << std::endl;
  std::cout << "General:" << std::endl;
  std::cout << "  help" << std::endl;
  std::cout << "  exit" << std::endl;
  std::cout << "---------------------------------" << std::endl;
  std::cout << "Note: Use quotes for multi-word titles and names." << std::endl;
}

// --- Main Application Logic ---

bool initializeServices(AppServices& services) {
  services.date_time_utils = std::make_shared<lms::utils::DateTimeUtils>();

  std::cout << "Welcome to the Library Management System!" << std::endl;
  std::cout << "Choose persistence type:" << std::endl;
  std::cout << "  1. In-Memory (data lost on exit)" << std::endl;
  std::cout << "  2. File-Based (CSV, data saved in ./lms_data/)" << std::endl;
  std::cout << "  3. Caching File-Based (In-memory with load/save to ./lms_data/)" << std::endl;
  std::cout << "  4. MS SQL Server (requires configured database and ODBC driver)" << std::endl;
  std::cout << "Enter choice (1-4): ";
  int choice = 0;
  if (!(std::cin >> choice)) {
    std::cerr << "Invalid input. Please enter a number." << std::endl;
    std::cin.clear();                                                    // Clear error flags
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Discard bad input
    return false;
  }
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::string data_path = "./lms_data/";

  try {
    switch (choice) {
      case 1:
        std::cout << "Using In-Memory Persistence." << std::endl;
        services.persistence_service_interface =
            std::make_shared<lms::persistence_service::InMemoryPersistenceService>();
        break;
      case 2:
        std::cout << "Using File-Based (CSV) Persistence in '" << data_path << "'." << std::endl;
        if (!std::filesystem::exists(data_path)) {
          std::filesystem::create_directories(data_path);
        }
        services.persistence_service_interface =
            std::make_shared<lms::persistence_service::FilePersistenceService>(
                data_path, services.date_time_utils);
        break;
      case 3:
        std::cout << "Using Caching File-Based Persistence (operates in memory, loads/saves to '"
                  << data_path << "')." << std::endl;
        if (!std::filesystem::exists(data_path)) {
          std::filesystem::create_directories(data_path);
        }
        services.caching_file_persistence_service =
            std::make_shared<lms::persistence_service::CachingFilePersistenceService>(
                data_path, services.date_time_utils);
        services.persistence_service_interface = services.caching_file_persistence_service;
        break;
      case 4: {
        std::cout << "Using MS SQL Server Persistence." << std::endl;
        std::string sql_conn_str;
        std::cout << "Enter MS SQL ODBC Connection String:" << std::endl;
        std::getline(std::cin, sql_conn_str);
        if (sql_conn_str.empty()) {
          std::cerr << "Connection string cannot be empty. Exiting." << std::endl;
          return false;
        }
        services.persistence_service_interface =
            std::make_shared<lms::persistence_service::MsSqlPersistenceService>(
                sql_conn_str, services.date_time_utils);
        std::cout << "Attempting to connect/initialize SQL persistence..." << std::endl;
        services.persistence_service_interface->loadAllAuthors();  // Trigger connection
        std::cout << "MS SQL Persistence initialized." << std::endl;
      } break;
      default:
        std::cerr << "Invalid choice for persistence type." << std::endl;
        return false;
    }
  } catch (const std::exception& e) {
    std::cerr << "ERROR during persistence service initialization: " << e.what() << std::endl;
    return false;
  }

  services.user_service =
      std::make_shared<lms::user_service::UserService>(services.persistence_service_interface);
  services.catalog_service = std::make_shared<lms::catalog_service::CatalogService>(
      services.persistence_service_interface);
  services.notification_service =
      std::make_shared<lms::notification_service::ConsoleNotificationService>();

  const int default_loan_duration_days = 14;
  services.loan_service = std::make_shared<lms::loan_service::LoanService>(
      services.catalog_service, services.user_service, services.persistence_service_interface,
      services.notification_service, services.date_time_utils, default_loan_duration_days);
  return true;
}

bool processCommand(const std::vector<std::string>& args,
                    AppServices& services,
                    bool& should_exit) {
  if (args.empty()) {
    return true;  // No command, continue loop
  }

  std::string command = args[0];
  std::transform(command.begin(), command.end(), command.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  try {
    if (command == "exit") {
      if (services.caching_file_persistence_service) {
        std::cout << "Save all changes to file before exiting? (yes/no): ";
        std::string save_choice;
        std::getline(std::cin, save_choice);
        std::transform(save_choice.begin(), save_choice.end(), save_choice.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (save_choice == "yes" || save_choice == "y") {
          services.caching_file_persistence_service->persistAllToFile();
        }
      }
      std::cout << "Exiting LMS." << std::endl;
      should_exit = true;
    } else if (command == "help") {
      printHelp();
    } else if (command == "saveall") {
      if (services.caching_file_persistence_service) {
        services.caching_file_persistence_service->persistAllToFile();
        std::cout << "All data from memory saved to files." << std::endl;
      } else {
        std::cout << "The 'saveAll' command is only available with Caching File-Based persistence."
                  << std::endl;
      }
    } else if (command == "adduser" && args.size() == 3) {
      services.user_service->addUser(args[1], args[2]);
      std::cout << "User '" << args[2] << "' with ID '" << args[1] << "' added." << std::endl;
    } else if (command == "finduser" && args.size() == 2) {
      auto user_opt = services.user_service->findUserById(args[1]);
      if (user_opt) {
        std::cout << "User found: ID=" << user_opt->getUserId() << ", Name=" << user_opt->getName()
                  << std::endl;
      } else {
        std::cout << "User with ID '" << args[1] << "' not found." << std::endl;
      }
    } else if (command == "listusers") {
      auto users = services.user_service->getAllUsers();
      if (users.empty()) {
        std::cout << "No users in the system." << std::endl;
      } else {
        std::cout << "Users:" << std::endl;
        for (const auto& user : users) {
          std::cout << "  ID: " << user.getUserId() << ", Name: " << user.getName() << std::endl;
        }
      }
    } else if (command == "addbook" && args.size() == 7) {
      int year = 0;
      try {
        year = std::stoi(args[6]);
      } catch (const std::exception& ex) {
        std::cerr << "Error: Invalid year '" << args[6] << "'. " << ex.what() << std::endl;
        return true;
      }
      services.catalog_service->addBook(args[1], args[2], args[3], args[4], args[5], year);
      std::cout << "Book '" << args[2] << "' added with ID '" << args[1] << "'." << std::endl;
    } else if (command == "finditem" && args.size() == 2) {
      auto item_opt_ptr = services.catalog_service->findItemById(args[1]);
      if (item_opt_ptr && item_opt_ptr.value()) {
        const auto& item = item_opt_ptr.value();
        std::cout << "Item found: ID=" << item->getId() << ", Title=" << item->getTitle()
                  << ", Status=" << availabilityStatusToString(item->getAvailabilityStatus());
        if (item->getAuthor()) {
          std::cout << ", Author=" << item->getAuthor()->getName();
        }
        if (const auto* book = dynamic_cast<const lms::domain_core::Book*>(item.get())) {
          std::cout << ", ISBN=" << book->getIsbn() << ", Year=" << book->getPublicationYear();
        }
        std::cout << std::endl;
      } else {
        std::cout << "Item with ID '" << args[1] << "' not found." << std::endl;
      }
    } else if (command == "listitems") {
      auto items = services.catalog_service->getAllItems();
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
              std::cout << ", ISBN=" << book->getIsbn() << ", Year=" << book->getPublicationYear();
            }
            std::cout << std::endl;
          }
        }
      }
    } else if (command == "borrow" && args.size() == 3) {
      lms::domain_core::LoanRecord loan = services.loan_service->borrowItem(args[1], args[2]);
      std::cout << "Item '" << args[2] << "' borrowed by user '" << args[1] << "'." << std::endl;
      std::cout << "  Loan ID: " << loan.getRecordId()
                << ", Due Date: " << services.date_time_utils->formatDate(loan.getDueDate())
                << std::endl;
    } else if (command == "return" && args.size() == 3) {
      services.loan_service->returnItem(args[1], args[2]);
      std::cout << "Item '" << args[2] << "' returned by user '" << args[1] << "'." << std::endl;
    } else if (command == "userloans" && args.size() == 2) {
      auto loans = services.loan_service->getActiveLoansForUser(args[1]);
      if (loans.empty()) {
        std::cout << "No active loans for user '" << args[1] << "'." << std::endl;
      } else {
        std::cout << "Active loans for user '" << args[1] << "':" << std::endl;
        for (const auto& loan : loans) {
          std::cout << "  Loan ID: " << loan.getRecordId() << ", Item ID: " << loan.getItemId()
                    << ", Loan Date: " << services.date_time_utils->formatDate(loan.getLoanDate())
                    << ", Due Date: " << services.date_time_utils->formatDate(loan.getDueDate())
                    << std::endl;
        }
      }
    } else if (command == "itemhistory" && args.size() == 2) {
      auto loans = services.loan_service->getLoanHistoryForItem(args[1]);
      if (loans.empty()) {
        std::cout << "No loan history for item '" << args[1] << "'." << std::endl;
      } else {
        std::cout << "Loan history for item '" << args[1] << "':" << std::endl;
        for (const auto& loan : loans) {
          std::cout << "  Loan ID: " << loan.getRecordId() << ", User ID: " << loan.getUserId()
                    << ", Loan Date: " << services.date_time_utils->formatDate(loan.getLoanDate())
                    << ", Due Date: " << services.date_time_utils->formatDate(loan.getDueDate());
          if (loan.getReturnDate().has_value()) {
            std::cout << ", Returned: "
                      << services.date_time_utils->formatDate(loan.getReturnDate().value());
          } else {
            std::cout << ", Status: Active";
          }
          std::cout << std::endl;
        }
      }
    } else if (command == "checkoverdue") {
      std::cout << "Checking for overdue items and sending notifications..." << std::endl;
      services.loan_service->processOverdueItems();
      std::cout << "Overdue check complete. Check console for notifications." << std::endl;
    } else {
      std::cout << "Unknown command or incorrect arguments. Type 'help' for commands." << std::endl;
    }
  } catch (const lms::domain_core::LmsException& e) {
    std::cerr << "LMS Error: " << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "System Error: " << e.what() << std::endl;
  }
  return true;  // Continue loop unless should_exit is true
}

void runCliLoop(AppServices& services) {
  std::string line;
  bool should_exit = false;

  printHelp();

  while (!should_exit) {
    std::cout << "\nlms> ";
    if (!std::getline(std::cin, line)) {
      should_exit = true;                               // EOF or error, treat as exit
      if (services.caching_file_persistence_service) {  // Auto-save on EOF with caching service
        std::cout << "\nEOF detected. Saving data..." << std::endl;
        services.caching_file_persistence_service->persistAllToFile();
      }
      std::cout << "Exiting LMS due to EOF or input error." << std::endl;
      break;
    }

    if (line.empty()) {
      continue;
    }

    std::vector<std::string> args = readCommandArgs(line);
    processCommand(args, services, should_exit);
  }
}

int main() {
  AppServices services;

  if (!initializeServices(services)) {
    std::cerr << "Application failed to initialize. Exiting." << std::endl;
    return 1;
  }

  runCliLoop(services);

  return 0;
}