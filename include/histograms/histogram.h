// Copyright (c) 2025 Bryan Kressler
//
// SPDX-License-Identifier: BSD-3-Clause
//
// Histogram: A helper class for working with bucketers to track value
// distributions.

#ifndef HISTOGRAMS_SRC_HISTOGRAM_H_
#define HISTOGRAMS_SRC_HISTOGRAM_H_

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace kressler::histograms {

// Histogram maintains counts of observations distributed across buckets
// defined by a Bucketer.
//
// Template parameters:
//   Bucketer: A bucketer type that provides:
//     - static size_t bucket(size_t value) - maps values to bucket indices
//     - static std::vector<size_t> bucket_boundaries() - returns bucket bounds
//
// Example usage:
//   using MyBucketer = LogLinearBucketer<100, 2, 1>;
//   Histogram<MyBucketer> hist;
//   hist.observe(42);
//   hist.observe(100, 5);  // Observe value 100 with count 5
//   auto results = hist.data();  // Get (boundary, count) pairs
template <typename Bucketer>
class Histogram {
 public:
  // Constructs a histogram with buckets defined by Bucketer.
  Histogram() : boundaries_(Bucketer::bucket_boundaries()) {
    data_.resize(boundaries_.size(), 0);
  }

  // Copy constructor - creates a deep copy of another histogram.
  Histogram(const Histogram& other)
      : boundaries_(other.boundaries_), data_(other.data_) {}

  // Merges another histogram into this one by summing bucket counts.
  // The histograms must have identical bucketing (same Bucketer type ensures
  // this).
  //
  // Parameters:
  //   other: The histogram to merge into this one
  //
  // Example:
  //   Histogram<MyBucketer> hist1, hist2;
  //   hist1.observe(10);
  //   hist2.observe(20);
  //   hist1.merge(hist2);  // hist1 now contains both observations
  void merge(const Histogram& other) {
    // Buckets should have the same size since they use the same Bucketer
    for (size_t i = 0; i < data_.size(); ++i) {
      data_[i] += other.data_[i];
    }
  }

  // Records an observation of the given value.
  // The value is mapped to a bucket using Bucketer::bucket(), and the
  // count for that bucket is incremented by n.
  //
  // Parameters:
  //   value: The value to observe
  //   n: The count to add (default: 1)
  void observe(size_t value, size_t n = 1) {
    const size_t bucket_idx = Bucketer::bucket(value);
    data_[bucket_idx] += n;
  }

  // Returns the histogram data as a vector of (boundary, count) pairs.
  //
  // Parameters:
  //   include_empty: If true, include buckets with zero counts (default: false)
  //
  // Returns:
  //   Vector of pairs where each pair is (bucket_lower_bound, count)
  std::vector<std::pair<size_t, size_t>> data(
      bool include_empty = false) const {
    std::vector<std::pair<size_t, size_t>> result;

    for (size_t i = 0; i < data_.size(); ++i) {
      if (include_empty || data_[i] > 0) {
        result.emplace_back(boundaries_[i], data_[i]);
      }
    }

    return result;
  }

  // Clears all observation counts, resetting the histogram to empty.
  void clear() {
    for (size_t& count : data_) {
      count = 0;
    }
  }

  // Returns the total number of observations across all buckets.
  size_t total_count() const {
    size_t total = 0;
    for (const size_t count : data_) {
      total += count;
    }
    return total;
  }

  // Computes percentile estimates for the given percentile values.
  // Uses linear interpolation within buckets to estimate percentiles.
  //
  // Parameters:
  //   percentiles: Vector of percentile values in the range [0.0, 1.0]
  //                (e.g., 0.5 for median, 0.95 for 95th percentile)
  //
  // Returns:
  //   Vector of estimated percentile values (same order as input)
  //   Returns empty vector if histogram is empty
  //
  // Example:
  //   auto p = hist.percentiles({0.5, 0.95, 0.99});  // median, p95, p99
  std::vector<double> percentiles(
      const std::vector<double>& percentiles_input) const {
    std::vector<double> result;
    result.reserve(percentiles_input.size());

    const size_t total = total_count();
    if (total == 0) {
      // Empty histogram - return empty result
      return result;
    }

    // Build cumulative counts
    std::vector<size_t> cumulative;
    cumulative.reserve(data_.size());
    size_t cumsum = 0;
    for (const size_t count : data_) {
      cumsum += count;
      cumulative.push_back(cumsum);
    }

    // Compute each percentile
    for (const double p : percentiles_input) {
      // Clamp percentile to [0, 1] range
      const double clamped_p = std::max(0.0, std::min(1.0, p));

      // Calculate target count (position in sorted observations)
      // For p=0, we want the first observation; for p=1, we want the last
      const double target_count = clamped_p * static_cast<double>(total);

      // Find bucket containing this count
      size_t bucket_idx = 0;
      if (target_count > 0.0) {
        // Find first bucket with cumulative count >= target
        for (size_t i = 0; i < cumulative.size(); ++i) {
          if (static_cast<double>(cumulative[i]) >= target_count) {
            bucket_idx = i;
            break;
          }
        }
      } else {
        // For p=0, find first non-empty bucket
        for (size_t i = 0; i < data_.size(); ++i) {
          if (data_[i] > 0) {
            bucket_idx = i;
            break;
          }
        }
      }

      // Interpolate within bucket
      const size_t lower_bound = boundaries_[bucket_idx];
      const size_t count_before =
          bucket_idx > 0 ? cumulative[bucket_idx - 1] : 0;
      const size_t count_in_bucket = data_[bucket_idx];

      if (count_in_bucket == 0) {
        // Empty bucket - just return lower bound
        result.push_back(static_cast<double>(lower_bound));
      } else if (bucket_idx + 1 < boundaries_.size()) {
        // Interpolate between lower and upper bounds
        const size_t upper_bound = boundaries_[bucket_idx + 1];
        const double position_in_bucket =
            (target_count - static_cast<double>(count_before)) /
            static_cast<double>(count_in_bucket);
        const double estimated_value =
            static_cast<double>(lower_bound) +
            position_in_bucket * static_cast<double>(upper_bound - lower_bound);
        result.push_back(estimated_value);
      } else {
        // Last bucket - no upper bound, return infinity
        result.push_back(std::numeric_limits<double>::infinity());
      }
    }

    return result;
  }

 private:
  // Bucket lower boundaries from the bucketer
  std::vector<size_t> boundaries_;

  // Count of observations in each bucket
  std::vector<size_t> data_;
};

}  // namespace kressler::histograms

#endif  // HISTOGRAMS_SRC_HISTOGRAM_H_
