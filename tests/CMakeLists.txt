# Test executables
add_executable(histogram_test histogram_test.cc)
target_link_libraries(histogram_test PRIVATE
  Catch2::Catch2WithMain
  histograms::histograms
)
add_test(NAME histogram_test COMMAND histogram_test)

add_executable(log_linear_bucketer_test log_linear_bucketer_test.cc)
target_link_libraries(log_linear_bucketer_test PRIVATE
  Catch2::Catch2WithMain
  histograms::histograms
)
add_test(NAME log_linear_bucketer_test COMMAND log_linear_bucketer_test)
