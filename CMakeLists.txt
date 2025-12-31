cmake_minimum_required(VERSION 3.30)
project(histograms)

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-mavx2)
    add_compile_options(-mfma)
    add_compile_options(-march=haswell)  # Haswell or later for AVX2
    add_compile_options(-mtune=native)
elseif(MSVC)
    message(STATUS "Building with AVX2 optimizations enabled (MSVC)")
    add_compile_options(/arch:AVX2)
endif()

# Option to enable AddressSanitizer (ASAN) for memory error detection
# Useful for debugging memory issues like buffer overflows, use-after-free, etc.
option(ENABLE_ASAN "Enable AddressSanitizer for memory error detection" OFF)

if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Building with AddressSanitizer (ASAN) enabled")
        add_compile_options(-fsanitize=address)
        add_compile_options(-fno-omit-frame-pointer)
        add_compile_options(-g)
        add_link_options(-fsanitize=address)

        # Disable optimizations that interfere with ASAN
        add_compile_options(-O1)
        add_compile_options(-fno-optimize-sibling-calls)
    elseif(MSVC)
        message(STATUS "Building with AddressSanitizer (ASAN) enabled (MSVC)")
        add_compile_options(/fsanitize=address)
        add_compile_options(/Zi)
    else()
        message(WARNING "AddressSanitizer not supported for this compiler")
    endif()
endif()

# Set aggressive optimization flags for Release builds only
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Aggressive optimizations
        add_compile_options(-O3)
        add_compile_options(-funroll-loops)
        add_compile_options(-fno-trapping-math)
        add_compile_options(-flto)
    elseif(MSVC)
        # MSVC optimizations
        add_compile_options(/O2)
    endif()
endif()

# Create INTERFACE library for header-only histogram library
add_library(histograms INTERFACE)
target_include_directories(histograms INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(histograms INTERFACE cxx_std_23)

# Export alias for consistent namespacing
add_library(kressler::histograms ALIAS histograms)

# Only build tests and development tools if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Enable testing
    enable_testing()

    # Add Catch2 testing framework
    add_subdirectory(third_party/Catch2)

    # Clang-format target
    find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
    if(CLANG_FORMAT_EXECUTABLE)
        file(GLOB_RECURSE ALL_SOURCE_FILES
            ${CMAKE_SOURCE_DIR}/include/*.h
            ${CMAKE_SOURCE_DIR}/include/*.hpp
            ${CMAKE_SOURCE_DIR}/tests/*.cc
            ${CMAKE_SOURCE_DIR}/tests/*.cpp
        )
        add_custom_target(
            format
            COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
            COMMENT "Running clang-format on all C++ source files"
            VERBATIM
        )
    else()
        message(WARNING "clang-format not found. 'format' target will not be available.")
    endif()

    # Add tests subdirectory
    add_subdirectory(tests)
endif()

# Install headers
install(FILES
    include/histograms/histogram.h
    include/histograms/log_linear_bucketer.h
    DESTINATION include/histograms
)

# Install targets
install(TARGETS histograms
    EXPORT histogramsTargets
    INCLUDES DESTINATION include
)

# Install export targets
install(EXPORT histogramsTargets
    FILE histogramsTargets.cmake
    NAMESPACE kressler::
    DESTINATION lib/cmake/histograms
)

# Create and install package config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/histogramsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/histogramsConfig.cmake
    INSTALL_DESTINATION lib/cmake/histograms
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/histogramsConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/histogramsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/histogramsConfigVersion.cmake
    DESTINATION lib/cmake/histograms
)
