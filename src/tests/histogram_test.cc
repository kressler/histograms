// Copyright 2025
// Tests for Histogram

#include "histogram.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "log_linear_bucketer.h"

using histograms::Histogram;
using histograms::LogLinearBucketer;

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
