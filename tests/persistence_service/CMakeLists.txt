# CMakeLists.txt for persistence_service tests

get_target_property(RUN_LMS_TESTS_SOURCES run_lms_tests SOURCES)

list(APPEND RUN_LMS_TESTS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/in_memory_persistence_service_test.cc
)
set_target_properties(run_lms_tests PROPERTIES SOURCES "${RUN_LMS_TESTS_SOURCES}")

# Tests for persistence_service need the persistence_service_lib
# and domain_core_lib (which persistence_service_lib already links publicly)
target_link_libraries(run_lms_tests PRIVATE persistence_service_lib)