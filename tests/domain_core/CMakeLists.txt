# CMakeLists.txt for domain_core tests

# List all your test source files for the domain_core component
set(DOMAIN_CORE_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/author_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/book_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/user_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/loan_record_test.cc
)

# Add these sources to the main test executable
# The tests will be compiled as part of the run_lms_tests target defined in tests/CMakeLists.txt
# We need to ensure the sources are added to the target.
# A common way is to have the parent CMakeLists.txt (tests/CMakeLists.txt)
# collect sources from subdirectories.
# Or, each test component creates its own library linked by run_lms_tests.
# For simplicity with a single test executable:
# Get the target run_lms_tests from the parent scope
get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

# Append current test sources
list(APPEND RUN_LMS_TESTS_SOURCES ${DOMAIN_CORE_TEST_SOURCES})
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Ensure this test module links against the domain_core library itself
target_link_libraries(run_lms_tests PRIVATE domain_core_lib)