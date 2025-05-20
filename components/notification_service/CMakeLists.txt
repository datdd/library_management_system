# CMakeLists.txt for notification_service component

add_library(notification_service_lib
    src/console_notification_service.cc
    # Add other notification implementations later if needed
)

target_include_directories(notification_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# notification_service depends on domain_core (for EntityId if used in messages, etc.)
# For now, its interface is simple and might not directly depend on domain_core types
# but let's link it in case future implementations need it or for consistency.
target_link_libraries(notification_service_lib
    PUBLIC
        domain_core_lib
)