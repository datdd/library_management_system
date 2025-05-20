# CMakeLists.txt for utils tests

# Get the target run_lms_tests from the parent scope
get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

# Append current test sources
list(APPEND RUN_LMS_TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/date_time_utils_test.cc
)
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Ensure this test module links against the utils library itself
target_link_libraries(run_lms_tests PRIVATE utils_lib)