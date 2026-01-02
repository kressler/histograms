#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace kressler::histograms {

// Linear bucketer with fixed-width buckets.
//
// Template parameters:
//   Buckets: Maximum number of buckets (values clamped to Buckets-1)
//   Min: Minimum value in scaled space (default: 0)
//   Scale: Scaling factor applied to values before bucketing (default: 1)
//
// Bucketing scheme:
//   - Values are scaled: scaled_value = value * Scale
//   - Bucket i contains scaled values in range [Min + i, Min + i + 1)
//   - Values < Min map to bucket 0
//   - Values >= Min + Buckets map to bucket Buckets - 1
//
// Example:
//   LinearBucketer<10, 0, 1>
//   - Bucket 0: [0, 1)
//   - Bucket 1: [1, 2)
//   - ...
//   - Bucket 9: [9, ∞)
template <size_t Buckets, size_t Min = 0, size_t Scale = 1>
class LinearBucketer {
 public:
  // Returns the bucket index for a given value.
  // Result is clamped to [0, Buckets-1].
  static constexpr size_t bucket(size_t value) noexcept {
    const size_t scaled = value * Scale;
    if (scaled < Min) {
      return 0;
    }
    const size_t offset = scaled - Min;
    return std::min(offset, Buckets - 1);
  }

  // Returns the lower boundaries of all buckets (in original value space).
  static std::vector<size_t> bucket_boundaries() {
    std::vector<size_t> boundaries;
    boundaries.reserve(Buckets);
    for (size_t i = 0; i < Buckets; ++i) {
      // Bucket i starts at scaled value (Min + i)
      // In original space: (Min + i) / Scale
      // Since we're using integer division, this represents the minimum
      // value that maps to this bucket
      const size_t scaled_boundary = Min + i;
      const size_t boundary = (scaled_boundary + Scale - 1) / Scale;
      boundaries.push_back(boundary);
    }
    return boundaries;
  }
};

}  // namespace kressler::histograms
