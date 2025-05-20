cmake_minimum_required(VERSION 3.15)
project(LibraryManagementSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Google Test ---
# Option to use system-installed GTest or FetchContent
option(LMS_USE_SYSTEM_GTEST "Use system-installed GTest" OFF)

if(LMS_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
    if(NOT GTest_FOUND)
        message(FATAL_ERROR "System GTest not found and LMS_USE_SYSTEM_GTEST is ON.")
    endif()
    # GTest::GTest and GTest::Main or gtest and gtest_main
    # This might vary based on how GTest is installed.
    # For simplicity, we'll rely on FetchContent mostly.
else()
    # Use FetchContent to get Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0 # Or a specific commit/tag
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# --- Doxygen ---
# find_package(Doxygen)
# if(DOXYGEN_FOUND)
#     set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
#     set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
#     set(DOXYGEN_PROJECT_NUMBER ${PROJECT_VERSION})
#     # You'd typically have a Doxyfile.in and configure_file it
#     # For now, we'll assume a Doxyfile exists at the root.
#     add_custom_target(docs ALL
#         COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
#         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#         COMMENT "Generating API documentation with Doxygen"
#         VERBATIM
#     )
# else()
#     message(WARNING "Doxygen not found. 'docs' target will not be available.")
# endif()

# --- Subdirectories ---
add_subdirectory(components)
add_subdirectory(app)

# Enable testing
enable_testing()
add_subdirectory(tests) # Add tests subdirectory last, as it might depend on other targets

# --- Global Compile Options (Example) ---
if(MSVC)
    add_compile_options(/W4 /WX) # Treat warnings as errors on MSVC
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror) # Treat warnings as errors
endif()

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")