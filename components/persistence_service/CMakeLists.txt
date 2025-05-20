# CMakeLists.txt for persistence_service component

add_library(persistence_service_lib
    src/in_memory_persistence_service.cc
    # Add other persistence implementations (e.g., file_persistence_service.cc) later
)

target_include_directories(persistence_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# persistence_service needs to know about the domain core entities
target_link_libraries(persistence_service_lib
    PUBLIC
        domain_core_lib # To use Author, Book, User, LoanRecord, ILibraryItem types
)

# If we use utils (e.g. for logging or specific file operations in other persistors)
# target_link_libraries(persistence_service_lib PUBLIC utils_lib)