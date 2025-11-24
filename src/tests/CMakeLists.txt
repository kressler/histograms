add_executable(log_linear_bucketer_test log_linear_bucketer_test.cc)

target_link_libraries(log_linear_bucketer_test PRIVATE Catch2::Catch2WithMain)

target_include_directories(log_linear_bucketer_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(histogram_test histogram_test.cc)

target_link_libraries(histogram_test PRIVATE Catch2::Catch2WithMain)

target_include_directories(histogram_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(Catch)
catch_discover_tests(log_linear_bucketer_test)
catch_discover_tests(histogram_test)
