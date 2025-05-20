# CMakeLists.txt for loan_service component

add_library(loan_service_lib
    src/loan_service.cc
)

target_include_directories(loan_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# loan_service depends on:
# - domain_core (for LoanRecord, EntityId, Date, etc.)
# - catalog_service (for ICatalogService interface)
# - user_service (for IUserService interface)
# - persistence_service (for IPersistenceService interface - to save/load LoanRecords)
# - notification_service (for INotificationService interface)
# - utils (for DateTimeUtils)
target_link_libraries(loan_service_lib
    PUBLIC
        domain_core_lib
        catalog_service_lib
        user_service_lib
        persistence_service_lib
        notification_service_lib
        utils_lib
)