# CMakeLists.txt for persistence_service component

add_library(persistence_service_lib
    src/in_memory_persistence_service.cc
    src/file_persistence_service.cc
    src/ms_sql_persistence_service.cc 
    src/caching_file_persistence_service.cc
    src/odbc_wrapper.cc
)

target_include_directories(persistence_service_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# --- Finding and Linking ODBC ---
# Use CMake's built-in module to find ODBC
find_package(ODBC)

if(ODBC_FOUND)
    message(STATUS "ODBC Found: TRUE")
    message(STATUS "ODBC Libraries: ${ODBC_LIBRARIES}")
    message(STATUS "ODBC Include Dirs: ${ODBC_INCLUDE_DIRS}")
    # Add ODBC include directories to your target if necessary
    # (often system paths are already picked up, but good to be explicit)
    target_include_directories(persistence_service_lib PRIVATE ${ODBC_INCLUDE_DIRS})
    # Link against the found ODBC libraries
    target_link_libraries(persistence_service_lib PUBLIC ${ODBC_LIBRARIES})
else()
    message(WARNING "ODBC package not found by CMake. You may need to install ODBC development libraries (e.g., unixodbc-dev on Linux, or ensure SQL Server ODBC Driver is installed and in PATH on Windows) or manually specify linking.")
    # Fallback linking attempt (platform-dependent, might not work universally)
    if(WIN32)
        # On Windows, odbc32.lib is typically the library for the ODBC Driver Manager
        # Specific drivers (like for SQL Server) are loaded at runtime by the Driver Manager.
        target_link_libraries(persistence_service_lib PUBLIC odbc32)
        # You might also need Userenv.lib for some ODBC functions depending on driver/usage
        # target_link_libraries(persistence_service_lib PUBLIC Userenv)
    elseif(UNIX AND NOT APPLE)
        # On Linux, it's typically -lodbc (linked as 'odbc')
        target_link_libraries(persistence_service_lib PUBLIC odbc)
    elseif(APPLE)
        # On macOS, iODBC or unixODBC might be used. Frameworks might also be involved.
        # This might require specific find_library calls or framework linking.
        # target_link_libraries(persistence_service_lib PUBLIC odbc) # Or specific framework
        message(WARNING "ODBC setup on macOS can be complex. Ensure iODBC or unixODBC dev files are installed.")
    endif()
endif()
# --- End ODBC ---


target_link_libraries(persistence_service_lib # Other dependencies
    PUBLIC
        domain_core_lib
        utils_lib 
)