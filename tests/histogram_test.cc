// Copyright (c) 2025 Bryan Kressler
//
// SPDX-License-Identifier: BSD-3-Clause
//
// Tests for Histogram

#include <histograms/histogram.h>

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include <histograms/log_linear_bucketer.h>

using kressler::histograms::Histogram;
using kressler::histograms::LogLinearBucketer;

TEST_CASE("Histogram basic functionality", "[histogram][basic]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Empty histogram returns no data") {
    auto result = hist.data();
    REQUIRE(result.empty());
  }

  SECTION("Single observation") {
    hist.observe(10);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 10);  // boundary
    REQUIRE(result[0].second == 1);  // count
  }

  SECTION("Multiple observations of same value") {
    hist.observe(10);
    hist.observe(10);
    hist.observe(10);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 10);
    REQUIRE(result[0].second == 3);
  }

  SECTION("Observations of different values in same bucket") {
    // Values 8 and 9 both map to bucket 8
    hist.observe(8);
    hist.observe(9);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 8);   // boundary of bucket 8
    REQUIRE(result[0].second == 2);  // total count
  }
}

TEST_CASE("Histogram observe with count parameter", "[histogram][observe]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Single observe with count > 1") {
    hist.observe(10, 5);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 10);
    REQUIRE(result[0].second == 5);
  }

  SECTION("Multiple observes with different counts") {
    hist.observe(10, 3);
    hist.observe(10, 2);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 10);
    REQUIRE(result[0].second == 5);
  }

  SECTION("observe with n=0 has no effect") {
    hist.observe(10, 0);
    auto result = hist.data();

    REQUIRE(result.empty());
  }
}

TEST_CASE("Histogram with multiple buckets", "[histogram][multi-bucket]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Observations across different buckets") {
    hist.observe(0);   // bucket 0
    hist.observe(5);   // bucket 5
    hist.observe(10);  // bucket 9
    hist.observe(20);  // bucket 13

    auto result = hist.data();
    REQUIRE(result.size() == 4);

    // Results should be in bucket order
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 1);

    REQUIRE(result[1].first == 5);
    REQUIRE(result[1].second == 1);

    REQUIRE(result[2].first == 10);
    REQUIRE(result[2].second == 1);

    REQUIRE(result[3].first == 20);
    REQUIRE(result[3].second == 1);
  }

  SECTION("Mixed counts across buckets") {
    hist.observe(0, 10);
    hist.observe(5, 5);
    hist.observe(10, 3);
    hist.observe(10, 2);

    auto result = hist.data();
    REQUIRE(result.size() == 3);

    REQUIRE(result[0].second == 10);
    REQUIRE(result[1].second == 5);
    REQUIRE(result[2].second == 5);
  }
}

TEST_CASE("Histogram clear functionality", "[histogram][clear]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  hist.observe(10, 5);
  hist.observe(20, 3);

  SECTION("Clear resets all counts") {
    hist.clear();
    auto result = hist.data();
    REQUIRE(result.empty());
  }

  SECTION("Can observe after clear") {
    hist.clear();
    hist.observe(30, 2);

    auto result = hist.data();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].second == 2);
  }
}

TEST_CASE("Histogram total_count", "[histogram][total_count]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Empty histogram has total_count 0") {
    REQUIRE(hist.total_count() == 0);
  }

  SECTION("Single observation") {
    hist.observe(10);
    REQUIRE(hist.total_count() == 1);
  }

  SECTION("Multiple observations") {
    hist.observe(10, 5);
    hist.observe(20, 3);
    hist.observe(30, 2);
    REQUIRE(hist.total_count() == 10);
  }

  SECTION("After clear, total_count is 0") {
    hist.observe(10, 100);
    hist.clear();
    REQUIRE(hist.total_count() == 0);
  }
}

TEST_CASE("Histogram with LogLinearBucketer integration",
          "[histogram][integration]") {
  SECTION("Bucketer with Scale parameter") {
    using Bucketer = LogLinearBucketer<100, 2, 1024>;
    Histogram<Bucketer> hist;

    // Values < 1024 map to bucket 0
    hist.observe(0);
    hist.observe(512);
    hist.observe(1023);

    auto result = hist.data();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 3);
  }

  SECTION("Different SignificantBits") {
    using Bucketer = LogLinearBucketer<200, 3, 1>;
    Histogram<Bucketer> hist;

    hist.observe(16);
    hist.observe(17);
    hist.observe(18);

    auto result = hist.data();
    // 16-17 in bucket 16, 18-19 in bucket 17
    REQUIRE(result.size() == 2);
    REQUIRE(result[0].first == 16);
    REQUIRE(result[0].second == 2);
    REQUIRE(result[1].first == 18);
    REQUIRE(result[1].second == 1);
  }
}

TEST_CASE("Histogram edge cases", "[histogram][edge-cases]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Observation of zero") {
    hist.observe(0);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 1);
  }

  SECTION("Large value observations") {
    hist.observe(1000000);
    auto result = hist.data();

    REQUIRE(result.size() == 1);
    REQUIRE(result[0].second == 1);
  }

  SECTION("Many observations") {
    for (size_t i = 0; i < 1000; ++i) {
      hist.observe(i);
    }

    auto result = hist.data();
    REQUIRE(result.size() > 0);
    REQUIRE(hist.total_count() == 1000);
  }
}

TEST_CASE("Histogram data ordering", "[histogram][ordering]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Data is returned in bucket order") {
    // Observe in reverse order
    hist.observe(100);
    hist.observe(50);
    hist.observe(20);
    hist.observe(10);
    hist.observe(5);

    auto result = hist.data();

    // Should be ordered by boundary (bucket index)
    for (size_t i = 1; i < result.size(); ++i) {
      REQUIRE(result[i].first > result[i - 1].first);
    }
  }
}

TEST_CASE("Histogram realistic usage scenario", "[histogram][realistic]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Latency histogram scenario") {
    // Simulate recording latencies (in microseconds)
    std::vector<size_t> latencies = {10, 12,  15,  20,  25,  30,  35,  40,  45,
                                     50, 100, 150, 200, 250, 300, 500, 1000};

    for (const size_t latency : latencies) {
      hist.observe(latency);
    }

    auto result = hist.data();

    // Should have multiple buckets
    REQUIRE(result.size() > 1);

    // Total count should match input
    REQUIRE(hist.total_count() == latencies.size());

    // All counts should sum to total
    size_t sum = 0;
    for (const auto& [boundary, count] : result) {
      sum += count;
    }
    REQUIRE(sum == latencies.size());
  }
}

TEST_CASE("Histogram data() with include_empty parameter",
          "[histogram][include_empty]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Default behavior filters zero-count buckets") {
    hist.observe(10);
    auto result = hist.data();

    // Should only include bucket with count > 0
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].first == 10);
    REQUIRE(result[0].second == 1);
  }

  SECTION("include_empty=false filters zero-count buckets") {
    hist.observe(10);
    auto result = hist.data(false);

    // Same as default
    REQUIRE(result.size() == 1);
  }

  SECTION("include_empty=true includes all buckets") {
    hist.observe(10);
    auto result = hist.data(true);

    // Should include all buckets from the bucketer
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(result.size() == boundaries.size());

    // First bucket should be 0 with count 0
    REQUIRE(result[0].first == 0);
    REQUIRE(result[0].second == 0);

    // Bucket 10 should have count 1
    bool found = false;
    for (const auto& [boundary, count] : result) {
      if (boundary == 10) {
        REQUIRE(count == 1);
        found = true;
      }
    }
    REQUIRE(found);
  }

  SECTION("include_empty=true with multiple observations") {
    hist.observe(5);
    hist.observe(10, 3);
    hist.observe(20, 2);

    auto result_filtered = hist.data(false);
    auto result_all = hist.data(true);

    // Filtered should only have 3 buckets
    REQUIRE(result_filtered.size() == 3);

    // All should have all boundaries
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(result_all.size() == boundaries.size());

    // Total counts should be same
    size_t sum_filtered = 0;
    for (const auto& [_, count] : result_filtered) {
      sum_filtered += count;
    }

    size_t sum_all = 0;
    for (const auto& [_, count] : result_all) {
      sum_all += count;
    }

    REQUIRE(sum_filtered == sum_all);
    REQUIRE(sum_all == 6);  // 1 + 3 + 2
  }
}

TEST_CASE("Histogram percentiles basic functionality",
          "[histogram][percentiles]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Empty histogram returns empty result") {
    auto result = hist.percentiles({0.5, 0.95, 0.99});
    REQUIRE(result.empty());
  }

  SECTION("Single observation") {
    hist.observe(10);
    auto result = hist.percentiles({0.0, 0.5, 1.0});

    REQUIRE(result.size() == 3);
    // All percentiles should be near the single value
    REQUIRE(result[0] >= 10.0);
    REQUIRE(result[1] >= 10.0);
    REQUIRE(result[2] >= 10.0);
  }

  SECTION("Multiple observations in same bucket") {
    // Values 8 and 9 both map to bucket 8
    hist.observe(8);
    hist.observe(9);
    auto result = hist.percentiles({0.5});

    REQUIRE(result.size() == 1);
    // Median should be between 8 and 10 (next bucket boundary)
    REQUIRE(result[0] >= 8.0);
    REQUIRE(result[0] < 10.0);
  }
}

TEST_CASE("Histogram percentiles with uniform distribution",
          "[histogram][percentiles]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Observations spread across buckets") {
    // Observe values 0-99 (100 observations)
    for (size_t i = 0; i < 100; ++i) {
      hist.observe(i);
    }

    auto result = hist.percentiles({0.0, 0.25, 0.5, 0.75, 1.0});

    REQUIRE(result.size() == 5);

    // p0 should be near 0
    REQUIRE(result[0] <= 5.0);

    // p25 should be roughly 25
    REQUIRE(result[1] >= 15.0);
    REQUIRE(result[1] <= 35.0);

    // p50 (median) should be roughly 50
    REQUIRE(result[2] >= 40.0);
    REQUIRE(result[2] <= 60.0);

    // p75 should be roughly 75
    REQUIRE(result[3] >= 65.0);
    REQUIRE(result[3] <= 85.0);

    // p100 should be high
    REQUIRE(result[4] >= 90.0);
  }
}

TEST_CASE("Histogram percentiles edge cases", "[histogram][percentiles]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Percentiles outside [0, 1] are clamped") {
    hist.observe(50, 100);

    auto result = hist.percentiles({-0.5, 1.5, 2.0});

    REQUIRE(result.size() == 3);
    // All should be clamped and return valid values
    REQUIRE(result[0] >= 0.0);
    REQUIRE(result[1] >= 0.0);
    REQUIRE(result[2] >= 0.0);
  }

  SECTION("Exact percentile boundaries") {
    // 100 observations at value 10
    hist.observe(10, 100);

    auto result = hist.percentiles({0.0, 1.0});

    REQUIRE(result.size() == 2);
    // Both should be in the same bucket
    REQUIRE(result[0] >= 10.0);
    REQUIRE(result[1] >= 10.0);
  }

  SECTION("Many percentiles at once") {
    for (size_t i = 0; i < 50; ++i) {
      hist.observe(i);
    }

    std::vector<double> percentiles;
    for (int i = 0; i <= 100; i += 10) {
      percentiles.push_back(i / 100.0);
    }

    auto result = hist.percentiles(percentiles);

    REQUIRE(result.size() == percentiles.size());
    // Results should be monotonically increasing
    for (size_t i = 1; i < result.size(); ++i) {
      REQUIRE(result[i] >= result[i - 1]);
    }
  }
}

TEST_CASE("Histogram percentiles with skewed distribution",
          "[histogram][percentiles]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Most observations at low values") {
    // 90 observations at 5, 10 observations at 100
    hist.observe(5, 90);
    hist.observe(100, 10);

    auto result = hist.percentiles({0.5, 0.9, 0.95});

    REQUIRE(result.size() == 3);

    // p50 should be in the low bucket (90% of data is there)
    REQUIRE(result[0] < 10.0);

    // p90 should still be in or near the low bucket
    REQUIRE(result[1] < 20.0);

    // p95 should be in the high bucket
    REQUIRE(result[2] > 50.0);
  }
}

TEST_CASE("Histogram percentiles realistic scenario",
          "[histogram][percentiles]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Latency percentiles") {
    // Simulate latency distribution (mostly low, some high outliers)
    for (int i = 0; i < 100; ++i) {
      hist.observe(10);  // 100 fast requests
    }
    for (int i = 0; i < 10; ++i) {
      hist.observe(50);  // 10 medium requests
    }
    hist.observe(200);  // 1 slow request

    auto result = hist.percentiles({0.5, 0.9, 0.95, 0.99});

    REQUIRE(result.size() == 4);

    // p50 should be around 10 (most requests are fast)
    REQUIRE(result[0] >= 8.0);
    REQUIRE(result[0] <= 15.0);

    // p90 should still be relatively low
    REQUIRE(result[1] >= 8.0);
    REQUIRE(result[1] <= 30.0);

    // p95 should start capturing medium latency
    REQUIRE(result[2] >= 10.0);
    REQUIRE(result[2] <= 60.0);

    // p99 should capture the slow request
    REQUIRE(result[3] >= 50.0);
  }
}

TEST_CASE("Histogram percentiles interpolation accuracy",
          "[histogram][percentiles]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist;

  SECTION("Known distribution for interpolation test") {
    // 10 observations in bucket starting at 10 (values would be ~10-12)
    hist.observe(10, 10);

    auto result = hist.percentiles({0.5});

    REQUIRE(result.size() == 1);
    // Median should be interpolated within the bucket [10, 12)
    REQUIRE(result[0] >= 10.0);
    REQUIRE(result[0] < 12.0);
  }

  SECTION("Last bucket returns max") {
    // Use a bucketer with very few buckets to easily hit the last one
    using SmallBucketer = LogLinearBucketer<10, 2, 1>;
    Histogram<SmallBucketer> small_hist;

    // Use a very large value that will clamp to the last bucket (bucket 9)
    small_hist.observe(1000000, 100);  // Very large value, many observations

    auto result = small_hist.percentiles({0.5, 0.95, 0.99});

    REQUIRE(result.size() == 3);
    // All percentiles fall in the last bucket, returns max() for unbounded
    REQUIRE(result[0] == std::numeric_limits<double>::max());
    REQUIRE(result[1] == std::numeric_limits<double>::max());
    REQUIRE(result[2] == std::numeric_limits<double>::max());
  }
}

TEST_CASE("Histogram copy constructor", "[histogram][copy]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;
  Histogram<Bucketer> hist1;

  SECTION("Copy empty histogram") {
    Histogram<Bucketer> hist2(hist1);

    REQUIRE(hist2.total_count() == 0);
    REQUIRE(hist2.data().empty());
  }

  SECTION("Copy histogram with observations") {
    hist1.observe(10, 5);
    hist1.observe(20, 3);
    hist1.observe(30, 2);

    Histogram<Bucketer> hist2(hist1);

    // Verify copied histogram has same data
    REQUIRE(hist2.total_count() == 10);

    auto data1 = hist1.data();
    auto data2 = hist2.data();

    REQUIRE(data1.size() == data2.size());
    for (size_t i = 0; i < data1.size(); ++i) {
      REQUIRE(data1[i].first == data2[i].first);
      REQUIRE(data1[i].second == data2[i].second);
    }
  }

  SECTION("Modifications to copy don't affect original") {
    hist1.observe(10, 5);

    Histogram<Bucketer> hist2(hist1);

    // Modify the copy
    hist2.observe(20, 3);

    // Original should be unchanged
    REQUIRE(hist1.total_count() == 5);
    REQUIRE(hist2.total_count() == 8);

    auto data1 = hist1.data();
    auto data2 = hist2.data();

    REQUIRE(data1.size() == 1);
    REQUIRE(data2.size() == 2);
  }

  SECTION("Clear on copy doesn't affect original") {
    hist1.observe(10, 5);

    Histogram<Bucketer> hist2(hist1);
    hist2.clear();

    REQUIRE(hist1.total_count() == 5);
    REQUIRE(hist2.total_count() == 0);
  }
}

TEST_CASE("Histogram merge method", "[histogram][merge]") {
  using Bucketer = LogLinearBucketer<100, 2, 1>;

  SECTION("Merge two empty histograms") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 0);
    REQUIRE(hist1.data().empty());
  }

  SECTION("Merge empty histogram into non-empty") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist1.observe(10, 5);

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 5);
    auto data = hist1.data();
    REQUIRE(data.size() == 1);
    REQUIRE(data[0].second == 5);
  }

  SECTION("Merge non-empty histogram into empty") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist2.observe(10, 5);

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 5);
    auto data = hist1.data();
    REQUIRE(data.size() == 1);
    REQUIRE(data[0].first == 10);
    REQUIRE(data[0].second == 5);
  }

  SECTION("Merge histograms with observations in same buckets") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist1.observe(10, 5);
    hist2.observe(10, 3);

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 8);
    auto data = hist1.data();
    REQUIRE(data.size() == 1);
    REQUIRE(data[0].first == 10);
    REQUIRE(data[0].second == 8);
  }

  SECTION("Merge histograms with observations in different buckets") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist1.observe(10, 5);
    hist2.observe(20, 3);

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 8);
    auto data = hist1.data();
    REQUIRE(data.size() == 2);
    REQUIRE(data[0].first == 10);
    REQUIRE(data[0].second == 5);
    REQUIRE(data[1].first == 20);
    REQUIRE(data[1].second == 3);
  }

  SECTION("Merge histograms with overlapping buckets") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist1.observe(10, 5);
    hist1.observe(20, 2);

    hist2.observe(10, 3);
    hist2.observe(30, 4);

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 14);  // 5 + 2 + 3 + 4
    auto data = hist1.data();
    REQUIRE(data.size() == 3);

    // Bucket 10: 5 + 3 = 8
    REQUIRE(data[0].first == 10);
    REQUIRE(data[0].second == 8);

    // Bucket 20: 2 + 0 = 2
    REQUIRE(data[1].first == 20);
    REQUIRE(data[1].second == 2);

    // Bucket 30: 0 + 4 = 4
    REQUIRE(data[2].first == 28);
    REQUIRE(data[2].second == 4);
  }

  SECTION("Merge doesn't modify the source histogram") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    hist1.observe(10, 5);
    hist2.observe(20, 3);

    hist1.merge(hist2);

    // hist2 should be unchanged
    REQUIRE(hist2.total_count() == 3);
    auto data2 = hist2.data();
    REQUIRE(data2.size() == 1);
    REQUIRE(data2[0].first == 20);
    REQUIRE(data2[0].second == 3);
  }

  SECTION("Multiple merges accumulate correctly") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;
    Histogram<Bucketer> hist3;

    hist1.observe(10, 5);
    hist2.observe(10, 3);
    hist3.observe(10, 2);

    hist1.merge(hist2);
    hist1.merge(hist3);

    REQUIRE(hist1.total_count() == 10);
    auto data = hist1.data();
    REQUIRE(data.size() == 1);
    REQUIRE(data[0].second == 10);
  }

  SECTION("Percentiles work correctly after merge") {
    Histogram<Bucketer> hist1;
    Histogram<Bucketer> hist2;

    // hist1: 50 observations at value 10
    hist1.observe(10, 50);

    // hist2: 50 observations at value 50
    hist2.observe(50, 50);

    hist1.merge(hist2);

    REQUIRE(hist1.total_count() == 100);

    auto result = hist1.percentiles({0.25, 0.5, 0.75});

    REQUIRE(result.size() == 3);
    // p25 should be in first bucket (value ~10)
    REQUIRE(result[0] < 20.0);
    // p50 should be at the boundary between buckets
    REQUIRE(result[1] > 10.0);
    REQUIRE(result[1] < 60.0);
    // p75 should be in second bucket (value ~50)
    REQUIRE(result[2] > 40.0);
  }
}
