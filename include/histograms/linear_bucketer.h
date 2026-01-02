#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace kressler::histograms {

// Linear bucketer with fixed-width buckets.
//
// Template parameters:
//   Buckets: Maximum number of buckets (values clamped to Buckets-1)
//   Min: Minimum value in unscaled space (default: 0)
//   Scale: Bucket width (default: 1)
//
// Bucketing scheme:
//   - Bucket i contains values in range [Min + i * Scale, Min + (i+1) * Scale)
//   - Values < Min map to bucket 0
//   - Values >= Min + Buckets * Scale map to bucket Buckets - 1
//
// Example:
//   LinearBucketer<20, 1000, 10>
//   - Bucket 0: [1000, 1010)
//   - Bucket 1: [1010, 1020)
//   - ...
//   - Bucket 19: [1190, ∞)
template <size_t Buckets, size_t Min = 0, size_t Scale = 1>
class LinearBucketer {
 public:
  // Returns the bucket index for a given value.
  // Result is clamped to [0, Buckets-1].
  static constexpr size_t bucket(size_t value) noexcept {
    if (value < Min) {
      return 0;
    }
    const size_t offset = (value - Min) / Scale;
    return std::min(offset, Buckets - 1);
  }

  // Returns the lower boundaries of all buckets.
  static std::vector<size_t> bucket_boundaries() {
    std::vector<size_t> boundaries;
    boundaries.reserve(Buckets);
    for (size_t i = 0; i < Buckets; ++i) {
      boundaries.push_back(Min + (i * Scale));
    }
    return boundaries;
  }
};

}  // namespace kressler::histograms
