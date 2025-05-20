# CMakeLists.txt for loan_service tests

get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

list(APPEND RUN_LMS_TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/loan_service_test.cc
)
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Tests for loan_service need loan_service_lib and GTest::gmock.
# loan_service_lib links all its dependencies.
target_link_libraries(run_lms_tests PRIVATE loan_service_lib GTest::gmock)
