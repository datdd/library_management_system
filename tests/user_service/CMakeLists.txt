# CMakeLists.txt for user_service tests

get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

list(APPEND RUN_LMS_TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/user_service_test.cc
)
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Tests for user_service need the user_service_lib
# and domain_core_lib, persistence_service_lib (which user_service_lib already links publicly)
# Also needs GTest::gmock for mocking.
target_link_libraries(run_lms_tests PRIVATE user_service_lib GTest::gmock)