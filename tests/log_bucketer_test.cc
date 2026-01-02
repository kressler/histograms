#include <histograms/log_bucketer.h>

#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace kressler::histograms;

TEST_CASE("LogBucketer - Basic bucketing", "[log_bucketer]") {
  using Bucketer = LogBucketer<20>;  // 20 buckets

  SECTION("Zero maps to bucket 0") { REQUIRE(Bucketer::bucket(0) == 0); }

  SECTION("One maps to bucket 1") { REQUIRE(Bucketer::bucket(1) == 1); }

  SECTION("Powers of 2 map to expected buckets") {
    // Bucket 0: 0
    // Bucket 1: 1
    // Bucket 2: [2, 4)
    // Bucket 3: [4, 8)
    // Bucket 4: [8, 16)
    REQUIRE(Bucketer::bucket(2) == 2);
    REQUIRE(Bucketer::bucket(4) == 3);
    REQUIRE(Bucketer::bucket(8) == 4);
    REQUIRE(Bucketer::bucket(16) == 5);
    REQUIRE(Bucketer::bucket(32) == 6);
    REQUIRE(Bucketer::bucket(64) == 7);
    REQUIRE(Bucketer::bucket(128) == 8);
    REQUIRE(Bucketer::bucket(256) == 9);
  }

  SECTION("Values between powers of 2 map correctly") {
    // Bucket 2: [2, 4)
    REQUIRE(Bucketer::bucket(2) == 2);
    REQUIRE(Bucketer::bucket(3) == 2);

    // Bucket 3: [4, 8)
    REQUIRE(Bucketer::bucket(4) == 3);
    REQUIRE(Bucketer::bucket(5) == 3);
    REQUIRE(Bucketer::bucket(6) == 3);
    REQUIRE(Bucketer::bucket(7) == 3);

    // Bucket 4: [8, 16)
    REQUIRE(Bucketer::bucket(8) == 4);
    REQUIRE(Bucketer::bucket(9) == 4);
    REQUIRE(Bucketer::bucket(15) == 4);

    // Bucket 5: [16, 32)
    REQUIRE(Bucketer::bucket(16) == 5);
    REQUIRE(Bucketer::bucket(31) == 5);
  }

  SECTION("Bucket boundaries are correct") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 20);

    REQUIRE(boundaries[0] == 0);
    REQUIRE(boundaries[1] == 1);
    REQUIRE(boundaries[2] == 2);    // 2^1
    REQUIRE(boundaries[3] == 4);    // 2^2
    REQUIRE(boundaries[4] == 8);    // 2^3
    REQUIRE(boundaries[5] == 16);   // 2^4
    REQUIRE(boundaries[6] == 32);   // 2^5
    REQUIRE(boundaries[7] == 64);   // 2^6
    REQUIRE(boundaries[8] == 128);  // 2^7
    REQUIRE(boundaries[9] == 256);  // 2^8
  }
}

TEST_CASE("LogBucketer - Clamping to max buckets", "[log_bucketer]") {
  using Bucketer = LogBucketer<5>;  // Only 5 buckets

  SECTION("Values within range map normally") {
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 1);
    REQUIRE(Bucketer::bucket(2) == 2);
    REQUIRE(Bucketer::bucket(3) == 2);
    REQUIRE(Bucketer::bucket(4) == 3);
    REQUIRE(Bucketer::bucket(7) == 3);
  }

  SECTION("Values beyond range clamp to last bucket") {
    // Bucket 4 would normally start at 2^3 = 8
    // But we only have 5 buckets (0-4)
    REQUIRE(Bucketer::bucket(8) == 4);
    REQUIRE(Bucketer::bucket(15) == 4);
    REQUIRE(Bucketer::bucket(16) == 4);
    REQUIRE(Bucketer::bucket(100) == 4);
    REQUIRE(Bucketer::bucket(1000) == 4);
    REQUIRE(Bucketer::bucket(1000000) == 4);
  }

  SECTION("Bucket boundaries stop at max buckets") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 5);
    REQUIRE(boundaries[0] == 0);
    REQUIRE(boundaries[1] == 1);
    REQUIRE(boundaries[2] == 2);  // 2^1
    REQUIRE(boundaries[3] == 4);  // 2^2
    REQUIRE(boundaries[4] == 8);  // 2^3
  }
}

TEST_CASE("LogBucketer - Edge cases", "[log_bucketer]") {
  SECTION("Single bucket") {
    using Bucketer = LogBucketer<1>;
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 0);
    REQUIRE(Bucketer::bucket(100) == 0);
    REQUIRE(Bucketer::bucket(1000000) == 0);

    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 1);
    REQUIRE(boundaries[0] == 0);
  }

  SECTION("Two buckets") {
    using Bucketer = LogBucketer<2>;
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 1);
    REQUIRE(Bucketer::bucket(2) == 1);
    REQUIRE(Bucketer::bucket(100) == 1);

    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 2);
    REQUIRE(boundaries[0] == 0);
    REQUIRE(boundaries[1] == 1);
  }

  SECTION("Three buckets") {
    using Bucketer = LogBucketer<3>;
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 1);
    REQUIRE(Bucketer::bucket(2) == 2);
    REQUIRE(Bucketer::bucket(3) == 2);
    REQUIRE(Bucketer::bucket(4) == 2);
    REQUIRE(Bucketer::bucket(100) == 2);

    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 3);
    REQUIRE(boundaries[0] == 0);
    REQUIRE(boundaries[1] == 1);
    REQUIRE(boundaries[2] == 2);
  }
}

TEST_CASE("LogBucketer - Large values", "[log_bucketer]") {
  using Bucketer = LogBucketer<64>;  // 64 buckets (enough for 64-bit values)

  SECTION("Large powers of 2 map correctly") {
    REQUIRE(Bucketer::bucket(1ULL << 10) == 11);  // 1024 -> bucket 11
    REQUIRE(Bucketer::bucket(1ULL << 20) == 21);  // 1M -> bucket 21
    REQUIRE(Bucketer::bucket(1ULL << 30) == 31);  // 1G -> bucket 31
    REQUIRE(Bucketer::bucket(1ULL << 40) == 41);  // 1T -> bucket 41
  }

  SECTION("Maximum 64-bit value") {
    // Maximum value that fits in size_t
    const size_t max_val = ~size_t{0};
    // bit_width(max_val) = 64
    REQUIRE(Bucketer::bucket(max_val) == 63);  // Clamped to last bucket
  }
}

TEST_CASE("LogBucketer - Constexpr bucket function", "[log_bucketer]") {
  using Bucketer = LogBucketer<10>;

  // Verify bucket() is constexpr by using it in a constant expression
  constexpr size_t b0 = Bucketer::bucket(0);
  constexpr size_t b1 = Bucketer::bucket(1);
  constexpr size_t b2 = Bucketer::bucket(2);
  constexpr size_t b4 = Bucketer::bucket(4);
  constexpr size_t b8 = Bucketer::bucket(8);

  REQUIRE(b0 == 0);
  REQUIRE(b1 == 1);
  REQUIRE(b2 == 2);
  REQUIRE(b4 == 3);
  REQUIRE(b8 == 4);
}

TEST_CASE("LogBucketer - Comparison with bit operations", "[log_bucketer]") {
  using Bucketer = LogBucketer<20>;

  // Verify our implementation matches expected bit_width behavior
  for (size_t val = 1; val < 1000; ++val) {
    size_t expected_bucket = std::bit_width(val);
    if (expected_bucket >= 20) {
      expected_bucket = 19;  // Clamp to last bucket
    }
    REQUIRE(Bucketer::bucket(val) == expected_bucket);
  }
}
