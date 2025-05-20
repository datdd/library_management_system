# CMakeLists.txt for tests directory

# This file will typically include a main for gtest if not linked directly
# and add subdirectories for component tests.

# Create a test executable that links GTest::Main
# (or gtest_main depending on GTest setup)
# This main_test.cc will be a simple GTest runner.
add_executable(run_lms_tests main_test.cc)
target_link_libraries(run_lms_tests PRIVATE GTest::gtest GTest::gmock GTest::gtest_main)
# If GTest::gtest_main is not found, you might need to link gtest and gtest_main separately
# or ensure your main_test.cc has int main(int argc, char **argv) { ... }

# Add CTest test entry
add_test(NAME AllLmsTests COMMAND run_lms_tests)


# Add subdirectories for specific component tests
add_subdirectory(domain_core)
add_subdirectory(utils)
add_subdirectory(persistence_service)
add_subdirectory(user_service)
add_subdirectory(catalog_service)
add_subdirectory(notification_service)
add_subdirectory(loan_service)

# Ensure test executable can find headers from components
target_include_directories(run_lms_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/components/domain_core/include # Access to domain_core headers
    # Add other component include directories as they are created
    ${CMAKE_SOURCE_DIR}/components/utils/include
    ${CMAKE_SOURCE_DIR}/components/persistence_service/include
    ${CMAKE_SOURCE_DIR}/components/user_service/include
    ${CMAKE_SOURCE_DIR}/components/catalog_service/include
    ${CMAKE_SOURCE_DIR}/components/notification_service/include
    ${CMAKE_SOURCE_DIR}/components/loan_service/include
    
    # Add any other directories needed for the tests
    # For example, if you have a common mock directory:
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)