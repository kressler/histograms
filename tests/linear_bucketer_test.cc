#include <histograms/linear_bucketer.h>

#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace kressler::histograms;

TEST_CASE("LinearBucketer - Basic bucketing with default parameters",
          "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10>;  // 10 buckets, Min=0, Scale=1

  SECTION("Values map to expected buckets") {
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 1);
    REQUIRE(Bucketer::bucket(2) == 2);
    REQUIRE(Bucketer::bucket(5) == 5);
    REQUIRE(Bucketer::bucket(9) == 9);
  }

  SECTION("Values beyond range clamp to last bucket") {
    REQUIRE(Bucketer::bucket(10) == 9);
    REQUIRE(Bucketer::bucket(100) == 9);
    REQUIRE(Bucketer::bucket(1000) == 9);
  }

  SECTION("Bucket boundaries are correct") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 10);
    REQUIRE(boundaries[0] == 0);
    REQUIRE(boundaries[1] == 1);
    REQUIRE(boundaries[2] == 2);
    REQUIRE(boundaries[9] == 9);
  }
}

TEST_CASE("LinearBucketer - With Min parameter", "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10, 5>;  // 10 buckets, Min=5, Scale=1

  SECTION("Values below Min map to bucket 0") {
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 0);
    REQUIRE(Bucketer::bucket(4) == 0);
  }

  SECTION("Values at and above Min map correctly") {
    REQUIRE(Bucketer::bucket(5) == 0);   // scaled=5, offset=0
    REQUIRE(Bucketer::bucket(6) == 1);   // scaled=6, offset=1
    REQUIRE(Bucketer::bucket(10) == 5);  // scaled=10, offset=5
    REQUIRE(Bucketer::bucket(14) == 9);  // scaled=14, offset=9
  }

  SECTION("Values beyond range clamp to last bucket") {
    REQUIRE(Bucketer::bucket(15) == 9);   // scaled=15, offset=10 -> clamp to 9
    REQUIRE(Bucketer::bucket(100) == 9);  // scaled=100, offset=95 -> clamp to 9
  }

  SECTION("Bucket boundaries account for Min") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 10);
    REQUIRE(boundaries[0] == 5);   // Min + 0
    REQUIRE(boundaries[1] == 6);   // Min + 1
    REQUIRE(boundaries[5] == 10);  // Min + 5
    REQUIRE(boundaries[9] == 14);  // Min + 9
  }
}

TEST_CASE("LinearBucketer - With Scale parameter", "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10, 0, 2>;  // 10 buckets, Min=0, Scale=2

  SECTION("Values are scaled before bucketing") {
    REQUIRE(Bucketer::bucket(0) == 0);  // scaled=0
    REQUIRE(Bucketer::bucket(1) == 2);  // scaled=2
    REQUIRE(Bucketer::bucket(2) == 4);  // scaled=4
    REQUIRE(Bucketer::bucket(3) == 6);  // scaled=6
    REQUIRE(Bucketer::bucket(4) == 8);  // scaled=8
  }

  SECTION("Values beyond range clamp to last bucket") {
    REQUIRE(Bucketer::bucket(5) == 9);   // scaled=10, offset=10 -> clamp to 9
    REQUIRE(Bucketer::bucket(10) == 9);  // scaled=20 -> clamp to 9
  }

  SECTION("Bucket boundaries account for Scale") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 10);
    // Bucket i starts at scaled value i, which is (i + Scale - 1) / Scale in
    // original space With Scale=2:
    //   Bucket 0: scaled [0, 1) -> original [0, 1)
    //   Bucket 1: scaled [1, 2) -> original [1, 1) (ceil(1/2) = 1)
    //   Bucket 2: scaled [2, 3) -> original [1, 2)
    REQUIRE(boundaries[0] == 0);  // (0 + 2 - 1) / 2 = 0
    REQUIRE(boundaries[1] == 1);  // (1 + 2 - 1) / 2 = 1
    REQUIRE(boundaries[2] == 1);  // (2 + 2 - 1) / 2 = 1
    REQUIRE(boundaries[3] == 2);  // (3 + 2 - 1) / 2 = 2
    REQUIRE(boundaries[4] == 2);  // (4 + 2 - 1) / 2 = 2
  }
}

TEST_CASE("LinearBucketer - With Min and Scale", "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10, 10, 2>;  // 10 buckets, Min=10, Scale=2

  SECTION("Values below Min map to bucket 0") {
    REQUIRE(Bucketer::bucket(0) == 0);  // scaled=0 < 10
    REQUIRE(Bucketer::bucket(4) == 0);  // scaled=8 < 10
  }

  SECTION("Values at and above Min map correctly") {
    REQUIRE(Bucketer::bucket(5) == 0);   // scaled=10, offset=0
    REQUIRE(Bucketer::bucket(6) == 2);   // scaled=12, offset=2
    REQUIRE(Bucketer::bucket(7) == 4);   // scaled=14, offset=4
    REQUIRE(Bucketer::bucket(10) == 9);  // scaled=20, offset=10 -> clamp to 9
  }

  SECTION("Bucket boundaries are correct") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 10);
    // Bucket i: scaled [10+i, 10+i+1) -> original (10+i + 2-1) / 2 = (11+i) / 2
    REQUIRE(boundaries[0] == 5);   // (10 + 2 - 1) / 2 = 11 / 2 = 5
    REQUIRE(boundaries[1] == 6);   // (11 + 2 - 1) / 2 = 12 / 2 = 6
    REQUIRE(boundaries[2] == 6);   // (12 + 2 - 1) / 2 = 13 / 2 = 6
    REQUIRE(boundaries[9] == 10);  // (19 + 2 - 1) / 2 = 20 / 2 = 10
  }
}

TEST_CASE("LinearBucketer - Edge cases", "[linear_bucketer]") {
  SECTION("Single bucket") {
    using Bucketer = LinearBucketer<1>;
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 0);
    REQUIRE(Bucketer::bucket(100) == 0);

    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 1);
    REQUIRE(boundaries[0] == 0);
  }

  SECTION("Two buckets") {
    using Bucketer = LinearBucketer<2>;
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(1) == 1);
    REQUIRE(Bucketer::bucket(2) == 1);
    REQUIRE(Bucketer::bucket(100) == 1);

    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 2);
    REQUIRE(boundaries[0] == 0);
    REQUIRE(boundaries[1] == 1);
  }
}

TEST_CASE("LinearBucketer - Constexpr bucket function", "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10, 5, 2>;

  // Verify bucket() is constexpr by using it in a constant expression
  constexpr size_t b0 = Bucketer::bucket(0);
  constexpr size_t b5 = Bucketer::bucket(5);
  constexpr size_t b10 = Bucketer::bucket(10);

  REQUIRE(b0 == 0);   // scaled=0 < 5
  REQUIRE(b5 == 5);   // scaled=10, offset=5, bucket=5
  REQUIRE(b10 == 9);  // scaled=20, offset=15, clamped to 9
}
