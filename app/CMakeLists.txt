# CMakeLists.txt for app component

add_executable(lms_app
    main.cc
    # Add other .cc files if you split CLI logic
)

# The app depends on all service libraries and domain_core/utils
target_link_libraries(lms_app
    PRIVATE
        domain_core_lib
        utils_lib
        persistence_service_lib
        user_service_lib
        catalog_service_lib
        notification_service_lib
        loan_service_lib
)

# If you install the application
# install(TARGETS lms_app DESTINATION bin)