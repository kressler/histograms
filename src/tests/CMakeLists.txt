add_executable(log_linear_bucketer_test log_linear_bucketer_test.cc)

target_link_libraries(log_linear_bucketer_test PRIVATE Catch2::Catch2WithMain)

target_include_directories(log_linear_bucketer_test
                           PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(Catch)
catch_discover_tests(log_linear_bucketer_test)
