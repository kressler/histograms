# Histograms

![CI](https://github.com/kressler/histograms/workflows/CI/badge.svg)

A high-performance, header-only C++23 library for tracking value distributions using log-linear bucketing.

## Features

- **Log-Linear Bucketing**: Maintains constant bits of precision across value ranges
- **Compile-Time Configuration**: Template-based bucketer with configurable bucket count, precision, and scaling
- **Percentile Estimation**: Compute percentiles (median, p95, p99) with linear interpolation
- **Header-Only**: Easy integration via CMake INTERFACE library
- **100% clang-tidy Clean**: Enforced code quality standards
- **C++23**: Modern C++ with full type safety

## Quick Start

### Basic Usage

```cpp
#include <histograms/histogram.h>
#include <histograms/log_linear_bucketer.h>

using namespace kressler::histograms;

// Define a bucketer: 100 buckets, 2 significant bits, scale=1
using Bucketer = LogLinearBucketer<100, 2, 1>;

// Create histogram
Histogram<Bucketer> hist;

// Record observations
hist.observe(42);
hist.observe(100, 5);  // Observe value 100 with count 5

// Get histogram data as (boundary, count) pairs
auto data = hist.data();
for (const auto& [boundary, count] : data) {
  std::cout << "Bucket " << boundary << ": " << count << " observations\n";
}

// Get total count
std::cout << "Total observations: " << hist.total_count() << "\n";

// Compute percentiles
auto percentiles = hist.percentiles({0.5, 0.95, 0.99});  // median, p95, p99
std::cout << "Median: " << percentiles[0] << "\n";
std::cout << "P95: " << percentiles[1] << "\n";
std::cout << "P99: " << percentiles[2] << "\n";
```

### Understanding LogLinearBucketer

The `LogLinearBucketer` uses a two-phase bucketing scheme:

1. **Linear Phase**: Small values map 1:1 to buckets
   - Range: `[0, 2^(SignificantBits+1) * Scale)`
   - Example: With `SignificantBits=2, Scale=1`, values 0-7 each get their own bucket

2. **Log-Linear Phase**: Larger values are grouped by precision
   - Each power-of-2 range is subdivided into `2^SignificantBits` buckets
   - Example: bucket(8-9) = 8, bucket(10-11) = 9

**Template Parameters:**
- `Buckets`: Maximum number of buckets (values clamped to Buckets-1)
- `SignificantBits`: Number of significant bits to maintain for precision
- `Scale`: Scaling factor applied before bucketing (default: 1)

**Example Configuration:**
```cpp
// 22 buckets, 2 significant bits, scale=1
LogLinearBucketer<22, 2, 1>
// Boundaries: {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, ...}
```

## Project Structure

```
histograms/
├── include/
│   └── histograms/
│       ├── histogram.h          # Histogram class template
│       └── log_linear_bucketer.h # LogLinearBucketer implementation
├── tests/
│   ├── histogram_test.cc        # Histogram tests
│   └── log_linear_bucketer_test.cc # Bucketer tests
├── third_party/
│   └── Catch2/                  # Testing framework (submodule)
├── hooks/                       # Git hooks (clang-format + clang-tidy)
├── setup-dev.sh                 # Development environment setup
└── CMakeLists.txt               # Build configuration
```

## Setup

### Prerequisites

- CMake 3.30+
- C++23 compiler (GCC 14+ or Clang 19+)
- clang-format (for code formatting)
- clang-tidy-19 (for static analysis)

### Clone and Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/kressler/histograms.git
cd histograms

# Set up development environment (one-time)
./setup-dev.sh
```

The `setup-dev.sh` script will:
- Install pre-commit hooks (clang-format + clang-tidy)
- Create `cmake-build-clang-tidy/` directory
- Verify required tools are installed

## Building and Testing

### Quick Build and Test

```bash
# Configure and build
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --parallel

# Run tests
ctest --test-dir cmake-build-debug --output-on-failure
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | - | Build type: `Debug` or `Release` |
| `ENABLE_ASAN` | `OFF` | Enable AddressSanitizer for memory error detection |

**Examples:**

```bash
# Debug build
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --parallel

# Release build
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --parallel

# ASAN build (for debugging memory issues)
cmake -S . -B cmake-build-asan -DENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel
ctest --test-dir cmake-build-asan --output-on-failure
```

## Code Quality

This project uses clang-format for code formatting and clang-tidy for static analysis.

### Code Formatting

Format all files:
```bash
cmake --build cmake-build-debug --target format
```

### Static Analysis

Run clang-tidy on all production headers:
```bash
cmake --build cmake-build-debug --target clang-tidy
```

Or manually:
```bash
clang-tidy-19 -p cmake-build-clang-tidy include/histograms/*.h
```

**Note**: Requires `cmake-build-clang-tidy/compile_commands.json` (created by `setup-dev.sh`)

### Pre-commit Hook

The pre-commit hook (installed by `setup-dev.sh`) automatically:
- Formats staged C++ files with clang-format
- Checks production headers with clang-tidy
- Auto-creates `cmake-build-clang-tidy/` if missing
- Blocks commit if clang-tidy warnings found

**Bypass hook** (when needed):
```bash
git commit --no-verify
```

## Using in Your Project

### Via CMake INTERFACE Library

```cmake
# Add histograms as a subdirectory
add_subdirectory(path/to/histograms)

# Link against the INTERFACE library
target_link_libraries(your_target PRIVATE kressler::histograms)
```

### Via CMake FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
  histograms
  GIT_REPOSITORY https://github.com/kressler/histograms.git
  GIT_TAG        main  # or specific commit/tag
)

FetchContent_MakeAvailable(histograms)

target_link_libraries(your_target PRIVATE kressler::histograms)
```

### Direct Include

Since it's header-only, you can also copy the headers:

```cpp
#include "path/to/histograms/histogram.h"
#include "path/to/histograms/log_linear_bucketer.h"
```

## API Reference

### Histogram<Bucketer>

**Constructor:**
```cpp
Histogram()  // Constructs with buckets defined by Bucketer
```

**Methods:**
```cpp
void observe(size_t value, size_t n = 1)
  // Records n observations of the given value

void merge(const Histogram& other)
  // Merges another histogram into this one by summing bucket counts

[[nodiscard]] std::vector<std::pair<size_t, size_t>> data(bool include_empty = false) const
  // Returns (boundary, count) pairs
  // include_empty: if true, include buckets with zero counts

void clear()
  // Clears all observation counts

[[nodiscard]] size_t total_count() const
  // Returns total number of observations across all buckets

[[nodiscard]] std::vector<double> percentiles(const std::vector<double>& percentiles) const
  // Computes percentile estimates using linear interpolation
  // percentiles: vector of values in range [0.0, 1.0] (e.g., 0.5 for median)
  // Returns: vector of estimated percentile values (same order as input)
```

### LogLinearBucketer<Buckets, SignificantBits, Scale>

**Static Methods:**
```cpp
static constexpr size_t bucket(size_t value) noexcept
  // Returns the bucket index for a given value
  // Result is clamped to [0, Buckets-1]

static std::vector<size_t> bucket_boundaries()
  // Returns the lower boundaries of all buckets
  // Vector has at most Buckets elements
```

## Development Workflow

1. Make your changes
2. Build and test:
   ```bash
   cmake --build cmake-build-debug && ctest --test-dir cmake-build-debug
   ```
3. Commit (pre-commit hook runs automatically):
   ```bash
   git add .
   git commit -m "Your changes"
   # Hook auto-formats code and checks with clang-tidy
   ```

## Code Conventions

- Follow Google C++ Style Guide (enforced by clang-format)
- Use C++23 features
- Write tests for new functionality using Catch2
- Production code must be clang-tidy clean (enforced in CI and pre-commit)
- Run `cmake --build cmake-build-debug --target format` before submitting PRs

## License

BSD 3-Clause License - see [LICENSE](LICENSE) for details

## Contributing

1. Run `./setup-dev.sh` to set up development environment
2. Make your changes following the code conventions
3. Add tests for new functionality
4. Ensure all tests pass and code is clang-tidy clean
5. Submit a pull request

## CI/CD

GitHub Actions CI automatically:
- Runs clang-tidy on all production headers
- Builds with GCC 14 (Debug/Release) and Clang 19 (Release)
- Runs all tests
- Fails if any clang-tidy warnings are found

See [.github/workflows/ci.yml](.github/workflows/ci.yml) for details.
