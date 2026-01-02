#pragma once

#include <bit>
#include <cstddef>
#include <vector>

namespace kressler::histograms {

// Log₂ bucketer with exponentially-sized buckets.
//
// Template parameters:
//   Buckets: Maximum number of buckets (values clamped to Buckets-1)
//
// Bucketing scheme:
//   - Bucket 0: value 0
//   - Bucket 1: value 1
//   - Bucket 2: values [2, 4)
//   - Bucket 3: values [4, 8)
//   - Bucket i (i >= 2): values [2^(i-1), 2^i)
//
// This is equivalent to LogLinearBucketer with 0 significant bits,
// providing pure log₂ bucketing.
//
// Example:
//   LogBucketer<10>
//   - Bucket 0: 0
//   - Bucket 1: 1
//   - Bucket 2: [2, 4)
//   - Bucket 3: [4, 8)
//   - Bucket 4: [8, 16)
//   - ...
//   - Bucket 9: [256, ∞)
template <size_t Buckets>
class LogBucketer {
 public:
  // Returns the bucket index for a given value using log₂ bucketing.
  // Result is clamped to [0, Buckets-1].
  static constexpr size_t bucket(size_t value) noexcept {
    if (value == 0) {
      return 0;
    }
    // std::bit_width(v) returns floor(log₂(v)) + 1 for v > 0
    // This gives us the desired bucket index
    const size_t bucket_idx = std::bit_width(value);
    return bucket_idx < Buckets ? bucket_idx : Buckets - 1;
  }

  // Returns the lower boundaries of all buckets.
  static std::vector<size_t> bucket_boundaries() {
    std::vector<size_t> boundaries;
    boundaries.reserve(Buckets);

    // Bucket 0: 0
    boundaries.push_back(0);

    if (Buckets > 1) {
      // Bucket 1: 1
      boundaries.push_back(1);
    }

    // Buckets i >= 2: 2^(i-1)
    for (size_t i = 2; i < Buckets; ++i) {
      boundaries.push_back(size_t{1} << (i - 1));
    }

    return boundaries;
  }
};

}  // namespace kressler::histograms
