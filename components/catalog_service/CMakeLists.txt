# CMakeLists.txt for catalog_service component

add_library(catalog_service_lib
    src/catalog_service.cc
)

target_include_directories(catalog_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# catalog_service depends on:
# - domain_core (for ILibraryItem, Book, Author types)
# - persistence_service (for IPersistenceService interface)
target_link_libraries(catalog_service_lib
    PUBLIC
        domain_core_lib
        persistence_service_lib
)

# Optionally, if it uses utils:
# target_link_libraries(catalog_service_lib PUBLIC utils_lib)