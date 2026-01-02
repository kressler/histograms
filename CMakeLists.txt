cmake_minimum_required(VERSION 3.30)
project(histograms)

# Create INTERFACE library for header-only histogram library
add_library(histograms INTERFACE)
add_library(kressler::histograms ALIAS histograms)

# Specify include directories
target_include_directories(histograms INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Require C++23
target_compile_features(histograms INTERFACE cxx_std_23)

# Only build tests if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install headers
install(FILES
    include/histograms/histogram.h
    include/histograms/log_linear_bucketer.h
    include/histograms/linear_bucketer.h
    include/histograms/log_bucketer.h
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
