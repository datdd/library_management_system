#include "notification_service/console_notification_service.h"  // Class to test
#include <iostream>             // For std::cout, std::cerr, std::streambuf
#include <sstream>              // For std::stringstream to capture output
#include "domain_core/types.h"  // For EntityId
#include "gtest/gtest.h"

using namespace lms::notification_service;
using namespace lms::domain_core;

// Helper class to temporarily redirect cout/cerr for testing console output
class OutputRedirector {
public:
  OutputRedirector(std::ostream& stream_to_redirect)
      : m_stream(stream_to_redirect), m_old_rdbuf(stream_to_redirect.rdbuf()) {
    m_stream.rdbuf(m_captured_output.rdbuf());
  }

  ~OutputRedirector() {
    m_stream.rdbuf(m_old_rdbuf);  // Restore original buffer
  }

  std::string getCapturedOutput() const { return m_captured_output.str(); }

private:
  std::ostream& m_stream;
  std::streambuf* m_old_rdbuf;
  std::stringstream m_captured_output;

  // Prevent copying
  OutputRedirector(const OutputRedirector&) = delete;
  OutputRedirector& operator=(const OutputRedirector&) = delete;
};

class ConsoleNotificationServiceTest : public ::testing::Test {
protected:
  ConsoleNotificationService service;
  // No complex setup needed for this simple service
};

TEST_F(ConsoleNotificationServiceTest, SendNotificationSuccessfully) {
  OutputRedirector cout_redirect(std::cout);  // Redirect std::cout

  EntityId user_id = "user_notify_1";
  std::string message = "Your book is due soon!";
  service.sendNotification(user_id, message);

  std::string expected_output = "[NOTIFICATION to User 'user_notify_1']: Your book is due soon!\n";
  EXPECT_EQ(cout_redirect.getCapturedOutput(), expected_output);
}

TEST_F(ConsoleNotificationServiceTest, SendNotificationWithEmptyUserIdPrintsError) {
  OutputRedirector cerr_redirect(std::cerr);  // Redirect std::cerr for error message

  EntityId user_id = "";
  std::string message = "This message won't be properly sent.";
  service.sendNotification(user_id, message);

  std::string expected_error_output =
      "[ConsoleNotificationService ERROR] User ID cannot be empty.\n";
  EXPECT_EQ(cerr_redirect.getCapturedOutput(), expected_error_output);

  // Also ensure nothing was printed to cout
  OutputRedirector cout_redirect(std::cout);
  service.sendNotification(user_id, message);  // Call again to check cout
  EXPECT_TRUE(cout_redirect.getCapturedOutput().empty());
}

TEST_F(ConsoleNotificationServiceTest, SendNotificationWithEmptyMessagePrintsError) {
  OutputRedirector cerr_redirect(std::cerr);  // Redirect std::cerr

  EntityId user_id = "user_notify_2";
  std::string message = "";
  service.sendNotification(user_id, message);

  std::string expected_error_output =
      "[ConsoleNotificationService ERROR] Notification message cannot be empty for user "
      "'user_notify_2'.\n";
  EXPECT_EQ(cerr_redirect.getCapturedOutput(), expected_error_output);

  // Also ensure nothing was printed to cout
  OutputRedirector cout_redirect(std::cout);
  service.sendNotification(user_id, message);  // Call again to check cout
  EXPECT_TRUE(cout_redirect.getCapturedOutput().empty());
}