# CMakeLists.txt for persistence_service component

add_library(persistence_service_lib
    src/in_memory_persistence_service.cc
    src/file_persistence_service.cc
    src/caching_file_persistence_service.cc
)

target_include_directories(persistence_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(persistence_service_lib
    PUBLIC
        domain_core_lib
        utils_lib # FilePersistenceService will use DateTimeUtils
)