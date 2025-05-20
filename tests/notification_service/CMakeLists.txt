# CMakeLists.txt for notification_service tests

get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

list(APPEND RUN_LMS_TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/console_notification_service_test.cc
)
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Tests for notification_service need the notification_service_lib
target_link_libraries(run_lms_tests PRIVATE notification_service_lib)