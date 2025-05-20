add_library(utils_lib # Changed from 'utils' to 'utils_lib' for clarity
    src/date_time_utils.cc
    # Add other .cc files here if any
)

target_include_directories(utils_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# utils_lib might depend on domain_core for 'Date' type if we don't redefine it.
# For now, let's assume it defines its own or uses std::chrono directly.
# If it needs domain_core::Date:
# target_link_libraries(utils_lib PUBLIC domain_core_lib)
# Let's make it self-contained for now and use std::chrono::system_clock::time_point directly.