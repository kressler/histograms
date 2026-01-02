# Add Catch2 testing framework
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Catch2 ${CMAKE_CURRENT_BINARY_DIR}/Catch2)

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

# Clang-tidy target
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-19 clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
    file(GLOB PRODUCTION_HEADERS
        ${CMAKE_SOURCE_DIR}/include/histograms/*.h
    )
    add_custom_target(
        clang-tidy
        COMMAND ${CLANG_TIDY_EXECUTABLE} -p ${CMAKE_SOURCE_DIR}/cmake-build-clang-tidy ${PRODUCTION_HEADERS}
        COMMENT "Running clang-tidy on production headers"
        VERBATIM
    )
else()
    message(WARNING "clang-tidy not found. 'clang-tidy' target will not be available.")
endif()

# Option to enable AddressSanitizer (ASAN) for memory error detection
# Useful for debugging memory issues like buffer overflows, use-after-free, etc.
option(ENABLE_ASAN "Enable AddressSanitizer for memory error detection" OFF)

if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Building tests with AddressSanitizer (ASAN) enabled")
        add_compile_options(-fsanitize=address)
        add_compile_options(-fno-omit-frame-pointer)
        add_compile_options(-g)
        add_link_options(-fsanitize=address)

        # Disable optimizations that interfere with ASAN
        add_compile_options(-O1)
        add_compile_options(-fno-optimize-sibling-calls)
    elseif(MSVC)
        message(STATUS "Building tests with AddressSanitizer (ASAN) enabled (MSVC)")
        add_compile_options(/fsanitize=address)
        add_compile_options(/Zi)
    else()
        message(WARNING "AddressSanitizer not supported for this compiler")
    endif()
endif()

# Enable -Werror to treat warnings as errors
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Werror)
elseif(MSVC)
    add_compile_options(/WX)
endif()

# Test executables
add_executable(histogram_test histogram_test.cc)
target_link_libraries(histogram_test PRIVATE
  Catch2::Catch2WithMain
  kressler::histograms
)
add_test(NAME histogram_test COMMAND histogram_test)

add_executable(log_linear_bucketer_test log_linear_bucketer_test.cc)
target_link_libraries(log_linear_bucketer_test PRIVATE
  Catch2::Catch2WithMain
  kressler::histograms
)
add_test(NAME log_linear_bucketer_test COMMAND log_linear_bucketer_test)

add_executable(linear_bucketer_test linear_bucketer_test.cc)
target_link_libraries(linear_bucketer_test PRIVATE
  Catch2::Catch2WithMain
  kressler::histograms
)
add_test(NAME linear_bucketer_test COMMAND linear_bucketer_test)

add_executable(log_bucketer_test log_bucketer_test.cc)
target_link_libraries(log_bucketer_test PRIVATE
  Catch2::Catch2WithMain
  kressler::histograms
)
add_test(NAME log_bucketer_test COMMAND log_bucketer_test)
