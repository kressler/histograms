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
  using Bucketer = LinearBucketer<10, 100>;  // 10 buckets, Min=100, Scale=1

  SECTION("Values below Min map to bucket 0") {
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(50) == 0);
    REQUIRE(Bucketer::bucket(99) == 0);
  }

  SECTION("Values at and above Min map correctly") {
    REQUIRE(Bucketer::bucket(100) == 0);  // offset=0
    REQUIRE(Bucketer::bucket(101) == 1);  // offset=1
    REQUIRE(Bucketer::bucket(105) == 5);  // offset=5
    REQUIRE(Bucketer::bucket(109) == 9);  // offset=9
  }

  SECTION("Values beyond range clamp to last bucket") {
    REQUIRE(Bucketer::bucket(110) == 9);   // offset=10 -> clamp to 9
    REQUIRE(Bucketer::bucket(200) == 9);   // offset=100 -> clamp to 9
    REQUIRE(Bucketer::bucket(1000) == 9);  // offset=900 -> clamp to 9
  }

  SECTION("Bucket boundaries account for Min") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 10);
    REQUIRE(boundaries[0] == 100);  // Min + 0 * Scale
    REQUIRE(boundaries[1] == 101);  // Min + 1 * Scale
    REQUIRE(boundaries[5] == 105);  // Min + 5 * Scale
    REQUIRE(boundaries[9] == 109);  // Min + 9 * Scale
  }
}

TEST_CASE("LinearBucketer - With Scale parameter", "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10, 0, 10>;  // 10 buckets, Min=0, Scale=10

  SECTION("Values are grouped by bucket width") {
    // Bucket 0: [0, 10)
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(5) == 0);
    REQUIRE(Bucketer::bucket(9) == 0);

    // Bucket 1: [10, 20)
    REQUIRE(Bucketer::bucket(10) == 1);
    REQUIRE(Bucketer::bucket(15) == 1);
    REQUIRE(Bucketer::bucket(19) == 1);

    // Bucket 5: [50, 60)
    REQUIRE(Bucketer::bucket(50) == 5);
    REQUIRE(Bucketer::bucket(59) == 5);

    // Bucket 9: [90, ∞)
    REQUIRE(Bucketer::bucket(90) == 9);
    REQUIRE(Bucketer::bucket(99) == 9);
  }

  SECTION("Values beyond range clamp to last bucket") {
    REQUIRE(Bucketer::bucket(100) == 9);   // offset=10 -> clamp to 9
    REQUIRE(Bucketer::bucket(1000) == 9);  // offset=100 -> clamp to 9
  }

  SECTION("Bucket boundaries account for Scale") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 10);
    REQUIRE(boundaries[0] == 0);   // 0 + 0 * 10
    REQUIRE(boundaries[1] == 10);  // 0 + 1 * 10
    REQUIRE(boundaries[2] == 20);  // 0 + 2 * 10
    REQUIRE(boundaries[5] == 50);  // 0 + 5 * 10
    REQUIRE(boundaries[9] == 90);  // 0 + 9 * 10
  }
}

TEST_CASE("LinearBucketer - With Min and Scale (user's example)",
          "[linear_bucketer]") {
  using Bucketer =
      LinearBucketer<20, 1000, 10>;  // 20 buckets of width 10 starting at 1000

  SECTION("Values below Min map to bucket 0") {
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(500) == 0);
    REQUIRE(Bucketer::bucket(999) == 0);
  }

  SECTION("Values map to expected buckets") {
    // Bucket 0: [1000, 1010)
    REQUIRE(Bucketer::bucket(1000) == 0);
    REQUIRE(Bucketer::bucket(1005) == 0);
    REQUIRE(Bucketer::bucket(1009) == 0);

    // Bucket 1: [1010, 1020)
    REQUIRE(Bucketer::bucket(1010) == 1);
    REQUIRE(Bucketer::bucket(1019) == 1);

    // Bucket 10: [1100, 1110)
    REQUIRE(Bucketer::bucket(1100) == 10);

    // Bucket 19: [1190, ∞)
    REQUIRE(Bucketer::bucket(1190) == 19);
    REQUIRE(Bucketer::bucket(1199) == 19);
  }

  SECTION("Values beyond range clamp to last bucket") {
    REQUIRE(Bucketer::bucket(1200) == 19);
    REQUIRE(Bucketer::bucket(2000) == 19);
  }

  SECTION("Bucket boundaries are correct") {
    auto boundaries = Bucketer::bucket_boundaries();
    REQUIRE(boundaries.size() == 20);
    REQUIRE(boundaries[0] == 1000);   // 1000 + 0 * 10
    REQUIRE(boundaries[1] == 1010);   // 1000 + 1 * 10
    REQUIRE(boundaries[10] == 1100);  // 1000 + 10 * 10
    REQUIRE(boundaries[19] == 1190);  // 1000 + 19 * 10
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

  SECTION("Large bucket width") {
    using Bucketer = LinearBucketer<10, 0, 100>;
    // Bucket 0: [0, 100), Bucket 1: [100, 200), etc.
    REQUIRE(Bucketer::bucket(0) == 0);
    REQUIRE(Bucketer::bucket(99) == 0);
    REQUIRE(Bucketer::bucket(100) == 1);
    REQUIRE(Bucketer::bucket(199) == 1);
    REQUIRE(Bucketer::bucket(900) == 9);
    REQUIRE(Bucketer::bucket(1000) == 9);  // clamp
  }
}

TEST_CASE("LinearBucketer - Constexpr bucket function", "[linear_bucketer]") {
  using Bucketer = LinearBucketer<10, 100, 10>;

  // Verify bucket() is constexpr by using it in a constant expression
  constexpr size_t b0 = Bucketer::bucket(0);
  constexpr size_t b100 = Bucketer::bucket(100);
  constexpr size_t b125 = Bucketer::bucket(125);
  constexpr size_t b200 = Bucketer::bucket(200);

  REQUIRE(b0 == 0);     // Below Min
  REQUIRE(b100 == 0);   // Min + 0 * Scale
  REQUIRE(b125 == 2);   // (125 - 100) / 10 = 2
  REQUIRE(b200 == 9);   // (200 - 100) / 10 = 10 -> clamp to 9
}
