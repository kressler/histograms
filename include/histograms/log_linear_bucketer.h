// Copyright (c) 2025 Bryan Kressler
//
// SPDX-License-Identifier: BSD-3-Clause
//
// LogLinearBucketer: A histogram bucketer maintaining constant bits of
// precision for histogram values using a log-linear bucketing scheme.

#ifndef HISTOGRAMS_SRC_LOG_LINEAR_BUCKETER_H_
#define HISTOGRAMS_SRC_LOG_LINEAR_BUCKETER_H_

#include <algorithm>
#include <bit>
#include <cstddef>
#include <vector>

namespace kressler::histograms {

// LogLinearBucketer implements a log-linear histogram bucketing scheme that
// maintains a constant number of bits of precision for histogrammed values.
//
// Template parameters:
//   Buckets: Maximum number of buckets (values will be clamped to Buckets-1)
//   SignificantBits: Number of significant bits to maintain
//   Scale: Scaling factor applied before bucketing (default: 1)
//
// The bucketing scheme has two phases:
//   1. Linear phase: Values [0, 2^(SignificantBits+1) * Scale) map 1:1
//   2. Log-linear phase: Each power-of-2 range is subdivided into
//      2^SignificantBits buckets based on the top SignificantBits
//
// Example: LogLinearBucketer<22, 2, 1>
//   - bucket(0-7) = 0-7 (linear)
//   - bucket(8-9) = 8, bucket(10-11) = 9 (log-linear)
//   - bucket_boundaries() = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, ...}
template <size_t Buckets, size_t SignificantBits, size_t Scale = 1>
class LogLinearBucketer {
 public:
  // Returns the bucket index for a given value.
  // The result is clamped to [0, Buckets-1].
  static constexpr size_t bucket(size_t value) noexcept {
    // Scale the value
    const size_t scaled = value / Scale;

    // Linear phase: small values (including 0) map 1:1 to bucket numbers
    if (scaled < kLinearThreshold) {
      return std::min(scaled, Buckets - 1);
    }

    // Log-linear phase: use bit manipulation to compute bucket
    // Find position of most significant bit (0-indexed from right)
    const size_t msb_pos = 63 - std::countl_zero(scaled);

    // Major index: which power-of-2 range we're in
    const size_t major_index = msb_pos - (SignificantBits + 1);

    // Extract the next SignificantBits after the MSB
    const size_t shift = msb_pos - SignificantBits;
    const size_t minor_index = (scaled >> shift) & kMinorMask;

    // Combine major and minor indices to get bucket number
    const size_t result =
        kLinearThreshold + (major_index << SignificantBits) + minor_index;

    // Clamp to maximum bucket
    return std::min(result, Buckets - 1);
  }

  // Returns the lower boundaries of all buckets.
  // The returned vector has at most Buckets elements, where each element
  // is the minimum value that maps to that bucket index.
  static std::vector<size_t> bucket_boundaries() {
    std::vector<size_t> boundaries;
    boundaries.reserve(Buckets);

    // Linear phase boundaries: [0, 1, 2, ..., kLinearThreshold-1]
    for (size_t i = 0; i < kLinearThreshold && i < Buckets; ++i) {
      boundaries.push_back(i * Scale);
    }

    // Log-linear phase boundaries
    size_t bucket_num = kLinearThreshold;
    for (size_t major = 0; bucket_num < Buckets; ++major) {
      for (size_t minor = 0;
           minor < (size_t{1} << SignificantBits) && bucket_num < Buckets;
           ++minor) {
        // Compute the MSB position for this major index
        const size_t msb_pos = major + SignificantBits + 1;

        // Base value: 2^msb_pos
        const size_t base = size_t{1} << msb_pos;

        // Offset within this power-of-2 range based on minor index
        const size_t offset = minor << (msb_pos - SignificantBits);

        boundaries.push_back((base + offset) * Scale);
        ++bucket_num;
      }
    }

    return boundaries;
  }

 private:
  // Threshold between linear and log-linear phases
  static constexpr size_t kLinearThreshold = size_t{1} << (SignificantBits + 1);

  // Mask to extract minor index bits
  static constexpr size_t kMinorMask = (size_t{1} << SignificantBits) - 1;

  // Compile-time validation of template parameters
  static_assert(Buckets > 0, "Buckets must be greater than 0");
  static_assert(Scale > 0, "Scale must be greater than 0");
  static_assert(SignificantBits > 0 && SignificantBits < 64,
                "SignificantBits must be in range (0, 64)");
};

}  // namespace kressler::histograms

#endif  // HISTOGRAMS_SRC_LOG_LINEAR_BUCKETER_H_
