# CMakeLists.txt for user_service component

add_library(user_service_lib
    src/user_service.cc
)

target_include_directories(user_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# user_service depends on domain_core (for User type)
# and persistence_service (for IPersistenceService interface)
target_link_libraries(user_service_lib
    PUBLIC
        domain_core_lib
        persistence_service_lib # For the IPersistenceService interface
)

# Optionally, if it uses utils:
# target_link_libraries(user_service_lib PUBLIC utils_lib)