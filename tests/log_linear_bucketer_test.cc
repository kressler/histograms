// Copyright 2025
// Tests for LogLinearBucketer

#include <histograms/log_linear_bucketer.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <limits>
#include <vector>

using kressler::histograms::LogLinearBucketer;

TEST_CASE("LogLinearBucketer basic functionality", "[basic]") {
  using B = LogLinearBucketer<100, 2, 1>;

  SECTION("Zero value maps to bucket 0") {
    REQUIRE(B::bucket(0) == 0);
  }

  SECTION("Linear range maps 1:1") {
    // For SignificantBits=2, linear threshold is 2^(2+1) = 8
    REQUIRE(B::bucket(1) == 1);
    REQUIRE(B::bucket(2) == 2);
    REQUIRE(B::bucket(3) == 3);
    REQUIRE(B::bucket(4) == 4);
    REQUIRE(B::bucket(5) == 5);
    REQUIRE(B::bucket(6) == 6);
    REQUIRE(B::bucket(7) == 7);
  }

  SECTION("First log-linear values") {
    REQUIRE(B::bucket(8) == 8);
    REQUIRE(B::bucket(9) == 8);
    REQUIRE(B::bucket(10) == 9);
    REQUIRE(B::bucket(11) == 9);
  }
}

TEST_CASE("Spec example: LogLinearBucketer<22, 2, 1>", "[spec]") {
  using B = LogLinearBucketer<22, 2, 1>;

  SECTION("Linear range (0-7)") {
    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(1) == 1);
    REQUIRE(B::bucket(2) == 2);
    REQUIRE(B::bucket(3) == 3);
    REQUIRE(B::bucket(4) == 4);
    REQUIRE(B::bucket(5) == 5);
    REQUIRE(B::bucket(6) == 6);
    REQUIRE(B::bucket(7) == 7);
  }

  SECTION("Log-linear range") {
    REQUIRE(B::bucket(8) == 8);
    REQUIRE(B::bucket(9) == 8);
    REQUIRE(B::bucket(10) == 9);
    REQUIRE(B::bucket(11) == 9);
    REQUIRE(B::bucket(12) == 10);
    REQUIRE(B::bucket(13) == 10);
    REQUIRE(B::bucket(14) == 11);
    REQUIRE(B::bucket(15) == 11);
    REQUIRE(B::bucket(16) == 12);
    REQUIRE(B::bucket(17) == 12);
    REQUIRE(B::bucket(18) == 12);
    REQUIRE(B::bucket(19) == 12);
    REQUIRE(B::bucket(20) == 13);
    REQUIRE(B::bucket(21) == 13);
  }

  SECTION("Bucket boundaries match spec") {
    auto boundaries = B::bucket_boundaries();
    std::vector<size_t> expected = {0,  1,  2,  3,  4,  5,  6,  7,  8,  10, 12,
                                    14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80};
    REQUIRE(boundaries == expected);
  }
}

TEST_CASE("LogLinearBucketer log-linear range", "[log-linear]") {
  using B = LogLinearBucketer<100, 2, 1>;

  SECTION("Power-of-2 boundaries") {
    // 8 = 2^3
    REQUIRE(B::bucket(8) == 8);
    // 16 = 2^4
    REQUIRE(B::bucket(16) == 12);
    // 32 = 2^5
    REQUIRE(B::bucket(32) == 16);
    // 64 = 2^6
    REQUIRE(B::bucket(64) == 20);
  }

  SECTION("Within-range values") {
    // Range [16, 32): subdivided into 4 buckets
    REQUIRE(B::bucket(16) == 12);
    REQUIRE(B::bucket(17) == 12);
    REQUIRE(B::bucket(19) == 12);
    REQUIRE(B::bucket(20) == 13);
    REQUIRE(B::bucket(23) == 13);
    REQUIRE(B::bucket(24) == 14);
    REQUIRE(B::bucket(27) == 14);
    REQUIRE(B::bucket(28) == 15);
    REQUIRE(B::bucket(31) == 15);
  }
}

TEST_CASE("LogLinearBucketer with different scales", "[scale]") {
  SECTION("Scale = 1024") {
    using B = LogLinearBucketer<100, 2, 1024>;

    // Values < 1024 map to bucket 0
    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(512) == 0);
    REQUIRE(B::bucket(1023) == 0);

    // Linear phase: [1024, 8192) maps to [1, 7]
    REQUIRE(B::bucket(1024) == 1);
    REQUIRE(B::bucket(2048) == 2);
    REQUIRE(B::bucket(7 * 1024) == 7);

    // Log-linear phase starts at 8 * 1024
    REQUIRE(B::bucket(8 * 1024) == 8);
    REQUIRE(B::bucket(9 * 1024) == 8);
  }

  SECTION("Scale = 4096") {
    using B = LogLinearBucketer<100, 3, 4096>;

    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(4095) == 0);
    REQUIRE(B::bucket(4096) == 1);
    REQUIRE(B::bucket(8192) == 2);
  }
}

TEST_CASE("LogLinearBucketer bucket boundaries", "[boundaries]") {
  using B = LogLinearBucketer<100, 2, 1>;

  SECTION("Boundaries are monotonically increasing") {
    auto boundaries = B::bucket_boundaries();
    for (size_t i = 1; i < boundaries.size(); ++i) {
      REQUIRE(boundaries[i] > boundaries[i - 1]);
    }
  }

  SECTION("Boundary count matches expected") {
    auto boundaries = B::bucket_boundaries();
    REQUIRE(boundaries.size() <= 100);
  }

  SECTION("Each boundary maps to its bucket index") {
    auto boundaries = B::bucket_boundaries();
    for (size_t i = 0; i < boundaries.size(); ++i) {
      REQUIRE(B::bucket(boundaries[i]) == i);
    }
  }

  SECTION("Values between boundaries map correctly") {
    auto boundaries = B::bucket_boundaries();
    for (size_t i = 0; i < boundaries.size() - 1; ++i) {
      size_t mid = (boundaries[i] + boundaries[i + 1]) / 2;
      if (mid > boundaries[i]) {
        REQUIRE(B::bucket(mid) == i);
      }
    }
  }

  SECTION("Boundary +1 and -1 mapping in log-linear range") {
    auto boundaries = B::bucket_boundaries();
    // Start from bucket 8 to ensure we're in log-linear range
    for (size_t i = 8; i < boundaries.size(); ++i) {
      size_t boundary = boundaries[i];

      // boundary + 1 should map to the same bucket (still in bucket i)
      if (i + 1 < boundaries.size() && boundary + 1 < boundaries[i + 1]) {
        REQUIRE(B::bucket(boundary + 1) == i);
      }

      // boundary - 1 should map to previous bucket (bucket i-1)
      if (i > 0 && boundary > 0) {
        REQUIRE(B::bucket(boundary - 1) == i - 1);
      }
    }
  }
}

TEST_CASE("LogLinearBucketer edge cases", "[edge-cases]") {
  SECTION("Large values") {
    using B = LogLinearBucketer<1000, 3, 1>;

    REQUIRE(B::bucket(1ULL << 20) < 1000);
    REQUIRE(B::bucket((1ULL << 32) - 1) < 1000);
    REQUIRE(B::bucket((1ULL << 63) - 1) < 1000);
  }

  SECTION("Bucket clamping when calculated bucket >= Buckets") {
    using B = LogLinearBucketer<10, 2, 1>;

    // Large values should clamp to bucket 9 (Buckets - 1)
    REQUIRE(B::bucket(1000) == 9);
    REQUIRE(B::bucket(1000000) == 9);
  }

  SECTION("All-linear configuration (Buckets <= kLinearThreshold)") {
    using B = LogLinearBucketer<8, 2, 1>;

    // Linear threshold is 2^(2+1) = 8, so all 8 buckets are linear
    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(7) == 7);
    REQUIRE(B::bucket(8) == 7);    // Clamped
    REQUIRE(B::bucket(100) == 7);  // Clamped

    auto boundaries = B::bucket_boundaries();
    REQUIRE(boundaries.size() == 8);
  }

  SECTION("Buckets < kLinearThreshold") {
    using B = LogLinearBucketer<5, 2, 1>;

    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(4) == 4);
    REQUIRE(B::bucket(5) == 4);   // Clamped
    REQUIRE(B::bucket(10) == 4);  // Clamped

    auto boundaries = B::bucket_boundaries();
    REQUIRE(boundaries.size() == 5);
  }
}

TEST_CASE("LogLinearBucketer with different SignificantBits",
          "[significant-bits]") {
  SECTION("SignificantBits = 1") {
    using B = LogLinearBucketer<100, 1, 1>;

    // Linear threshold: 2^(1+1) = 4
    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(3) == 3);

    // Log-linear: 2 subdivisions per power-of-2 range
    REQUIRE(B::bucket(4) == 4);
    REQUIRE(B::bucket(5) == 4);
    REQUIRE(B::bucket(6) == 5);
    REQUIRE(B::bucket(7) == 5);
  }

  SECTION("SignificantBits = 3") {
    using B = LogLinearBucketer<200, 3, 1>;

    // Linear threshold: 2^(3+1) = 16
    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(15) == 15);

    // Log-linear: 8 subdivisions per power-of-2 range [16, 32)
    REQUIRE(B::bucket(16) == 16);  // Subdivision 0: [16, 18)
    REQUIRE(B::bucket(17) == 16);
    REQUIRE(B::bucket(18) == 17);  // Subdivision 1: [18, 20)
    REQUIRE(B::bucket(19) == 17);
    REQUIRE(B::bucket(20) == 18);  // Subdivision 2: [20, 22)
    REQUIRE(B::bucket(21) == 18);
    REQUIRE(B::bucket(22) == 19);  // Subdivision 3: [22, 24)
    REQUIRE(B::bucket(23) == 19);
    REQUIRE(B::bucket(24) == 20);  // Subdivision 4: [24, 26)
    REQUIRE(B::bucket(25) == 20);
    REQUIRE(B::bucket(26) == 21);  // Subdivision 5: [26, 28)
    REQUIRE(B::bucket(27) == 21);
    REQUIRE(B::bucket(28) == 22);  // Subdivision 6: [28, 30)
    REQUIRE(B::bucket(29) == 22);
    REQUIRE(B::bucket(30) == 23);  // Subdivision 7: [30, 32)
    REQUIRE(B::bucket(31) == 23);
    REQUIRE(B::bucket(32) == 24);  // Next range starts
  }

  SECTION("SignificantBits = 4") {
    using B = LogLinearBucketer<500, 4, 1>;

    // Linear threshold: 2^(4+1) = 32
    REQUIRE(B::bucket(0) == 0);
    REQUIRE(B::bucket(31) == 31);

    // Log-linear: 16 subdivisions per power-of-2 range [32, 64)
    REQUIRE(B::bucket(32) == 32);  // Subdivision 0: [32, 34)
    REQUIRE(B::bucket(33) == 32);
    REQUIRE(B::bucket(34) == 33);  // Subdivision 1: [34, 36)
    REQUIRE(B::bucket(35) == 33);
    REQUIRE(B::bucket(36) == 34);  // Subdivision 2: [36, 38)
    REQUIRE(B::bucket(37) == 34);
    REQUIRE(B::bucket(38) == 35);  // Subdivision 3: [38, 40)
    REQUIRE(B::bucket(39) == 35);
    REQUIRE(B::bucket(40) == 36);  // Subdivision 4: [40, 42)
    REQUIRE(B::bucket(42) == 37);  // Subdivision 5: [42, 44)
    REQUIRE(B::bucket(44) == 38);  // Subdivision 6: [44, 46)
    REQUIRE(B::bucket(46) == 39);  // Subdivision 7: [46, 48)
    REQUIRE(B::bucket(48) == 40);  // Subdivision 8: [48, 50)
    REQUIRE(B::bucket(50) == 41);  // Subdivision 9: [50, 52)
    REQUIRE(B::bucket(52) == 42);  // Subdivision 10: [52, 54)
    REQUIRE(B::bucket(54) == 43);  // Subdivision 11: [54, 56)
    REQUIRE(B::bucket(56) == 44);  // Subdivision 12: [56, 58)
    REQUIRE(B::bucket(58) == 45);  // Subdivision 13: [58, 60)
    REQUIRE(B::bucket(60) == 46);  // Subdivision 14: [60, 62)
    REQUIRE(B::bucket(62) == 47);  // Subdivision 15: [62, 64)
    REQUIRE(B::bucket(64) == 48);  // Next range starts
  }
}

TEST_CASE("LogLinearBucketer constexpr evaluation", "[constexpr]") {
  SECTION("bucket() is constexpr") {
    using B = LogLinearBucketer<100, 2, 1>;

    // Compile-time evaluation
    constexpr size_t bucket_0 = B::bucket(0);
    constexpr size_t bucket_7 = B::bucket(7);
    constexpr size_t bucket_10 = B::bucket(10);

    REQUIRE(bucket_0 == 0);
    REQUIRE(bucket_7 == 7);
    REQUIRE(bucket_10 == 9);
  }
}

TEST_CASE("LogLinearBucketer comprehensive boundary validation",
          "[validation]") {
  using B = LogLinearBucketer<100, 2, 1>;

  auto boundaries = B::bucket_boundaries();

  SECTION("Every value maps to correct bucket based on boundaries") {
    // Test a range of values
    for (size_t value = 0; value <= 1000; ++value) {
      size_t bucket_idx = B::bucket(value);

      // Value should be >= its bucket's lower boundary
      REQUIRE(value >= boundaries[bucket_idx]);

      // Value should be < next bucket's lower boundary (if not last bucket)
      if (bucket_idx + 1 < boundaries.size()) {
        REQUIRE(value < boundaries[bucket_idx + 1]);
      }
    }
  }
}
