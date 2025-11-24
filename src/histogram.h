// Copyright 2025
// Histogram: A helper class for working with bucketers to track value
// distributions.

#ifndef HISTOGRAMS_SRC_HISTOGRAM_H_
#define HISTOGRAMS_SRC_HISTOGRAM_H_

#include <cstddef>
#include <utility>
#include <vector>

namespace histograms {

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

  // Records an observation of the given value.
  // The value is mapped to a bucket using Bucketer::bucket(), and the
  // count for that bucket is incremented by n.
  //
  // Parameters:
  //   value: The value to observe
  //   n: The count to add (default: 1)
  void observe(size_t value, size_t n = 1) {
    const size_t bucket_idx = Bucketer::bucket(value);
    if (bucket_idx < data_.size()) {
      data_[bucket_idx] += n;
    }
  }

  // Returns the histogram data as a vector of (boundary, count) pairs.
  // Only buckets with non-zero counts are included.
  //
  // Returns:
  //   Vector of pairs where each pair is (bucket_lower_bound, count)
  std::vector<std::pair<size_t, size_t>> data() const {
    std::vector<std::pair<size_t, size_t>> result;

    for (size_t i = 0; i < data_.size(); ++i) {
      if (data_[i] > 0) {
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

 private:
  // Bucket lower boundaries from the bucketer
  std::vector<size_t> boundaries_;

  // Count of observations in each bucket
  std::vector<size_t> data_;
};

}  // namespace histograms

#endif  // HISTOGRAMS_SRC_HISTOGRAM_H_
