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
        add_compile_options(-ffast-math)
        add_compile_options(-funroll-loops)
        add_compile_options(-fno-trapping-math)
        add_compile_options(-flto)
    elseif(MSVC)
        # MSVC optimizations
        add_compile_options(/O2 /fp:fast)
    endif()
endif()

# Enable testing
enable_testing()

# Clang-format target
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
        ${CMAKE_SOURCE_DIR}/src/*.ipp
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

add_subdirectory(src)
