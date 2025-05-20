# CMakeLists.txt for domain_core component

add_library(domain_core_lib # Renaming to avoid conflict if INTERFACE was used first
    src/author.cc
    src/book.cc
    src/user.cc
    src/loan_record.cc
)

target_include_directories(domain_core_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Alias for easier usage if desired
# add_library(domain_core ALIAS domain_core_lib)
# Using domain_core_lib directly is fine.