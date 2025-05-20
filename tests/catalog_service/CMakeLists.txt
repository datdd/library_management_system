# CMakeLists.txt for catalog_service tests

get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

list(APPEND RUN_LMS_TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/catalog_service_test.cc
)
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Tests for catalog_service need catalog_service_lib
# and GTest::gmock. It also needs access to the mock persistence service header.
# catalog_service_lib already links domain_core and persistence_service.
target_link_libraries(run_lms_tests PRIVATE catalog_service_lib GTest::gmock)