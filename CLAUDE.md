# Histograms - C++23 Header-Only Library

## Overview

High-performance, header-only C++23 library for tracking value distributions with flexible bucketing strategies.

**Namespace**: `kressler::histograms`
**Standard**: C++23
**Style Guide**: Google C++ Style
**Testing**: Catch2 v3 (git submodule)

## Repository Structure
```
include/histograms/              # Public API headers
  histogram.h                    # Histogram<Bucketer> template class
  log_linear_bucketer.h          # LogLinearBucketer<Buckets, SigBits, Scale>
  linear_bucketer.h              # LinearBucketer<Buckets, Min, Scale>
  log_bucketer.h                 # LogBucketer<Buckets>
tests/                           # Unit tests (Catch2)
  histogram_test.cc
  log_linear_bucketer_test.cc
  linear_bucketer_test.cc
  log_bucketer_test.cc
cmake/
  histogramsConfig.cmake.in      # CMake package config
third_party/Catch2/              # Git submodule
hooks/pre-commit                 # Auto-format and clang-tidy
setup-dev.sh                     # One-time development setup
CMakeLists.txt
```

## Core Components

### Histogram<Bucketer>
Generic histogram template that works with any bucketer.

**Methods**:
- `observe(value, n=1)` - No bounds check, trusts bucketer
- `data(include_empty=false)` - Returns vector of (boundary, count) pairs
- `clear()` - Reset all counts
- `total_count()` - Sum of all observations
- `percentiles(vector<double>)` - Linear interpolation, returns max() for last bucket

### LogLinearBucketer<Buckets, SignificantBits, Scale>
Maintains constant precision across value ranges using two-phase bucketing.

**Phases**:
1. **Linear**: Values [0, 2^(SignificantBits+1) * Scale) map 1:1 to buckets
2. **Log-linear**: Larger values grouped by precision (2^SignificantBits buckets per power-of-2 range)

**Use when**: Need consistent relative precision across wide ranges (e.g., latency distributions)

### LinearBucketer<Buckets, Min, Scale>
Fixed-width bucketing with configurable range.

**Bucketing**: Bucket i contains values `[Min + i * Scale, Min + (i+1) * Scale)`

**Use when**: Need equal-width bins over known range (e.g., temperature from 1000-1200°C in 10° increments)

### LogBucketer<Buckets>
Pure log₂ bucketing with exponentially-sized buckets.

**Bucketing**: Bucket i (i ≥ 2) contains values `[2^(i-1), 2^i)`

**Use when**: Need exponential bucketing without precision control (simpler than LogLinearBucketer)

## Build & Test

### Quick Commands
```bash
# Debug build
cmake -B cmake-build-debug && cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug

# Release build
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
ctest --test-dir cmake-build-release

# ASAN build (memory debugging)
cmake -B cmake-build-asan -DENABLE_ASAN=ON
cmake --build cmake-build-asan
ctest --test-dir cmake-build-asan

# Format code
cmake --build cmake-build-debug --target format

# Run clang-tidy
cmake --build build --target clang-tidy
```

### Build Options
- `ENABLE_ASAN=ON` - AddressSanitizer for memory debugging
- `CMAKE_BUILD_TYPE=Release` - Aggressive optimizations (-O3, -ffast-math, -flto)

## Development Workflow

### Setup
```bash
git clone --recursive https://github.com/kressler/histograms.git
cd histograms
./setup-dev.sh  # Installs pre-commit hooks (auto-format + clang-tidy)
```

### Pre-commit Hook
Automatically formats .cc/.h files with clang-format and runs clang-tidy on production headers. Blocks commit if warnings found.

### Branch Naming
- `feature/descriptive-name` for new features
- Work on branches, create PRs to `main`

### Commit Style
```
Brief imperative description

More detailed explanation if needed.

Changes:
- Bullet points of specific changes

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

## Code Conventions

### Always
- Mark all intermediate variables `const` where possible
- Use `size_t` for indices, counts, and values
- Use `constexpr` for compile-time constants
- Prefer `noexcept` on performance-critical paths
- Detailed comments for template parameters and methods
- Example usage in class-level comments

### Never
- Don't add bounds checking in hot paths (trust the API contract)
- Don't add unnecessary zero checks (removed for performance)
- Don't use raw loops where STL algorithms apply
- Don't add features beyond what's requested (avoid over-engineering)

### Testing
- Use Catch2 `SECTION()` for test organization
- Test boundary conditions (0, 1, max values)
- Test +1/-1 around boundaries
- Test edge cases (empty histogram, single bucket, last bucket)
- Verify algorithm correctness with concrete examples

## CMake Integration

### Consuming the Library

**As subdirectory**:
```cmake
add_subdirectory(third_party/histograms)
target_link_libraries(my_target PRIVATE kressler::histograms)
```

**After installation**:
```cmake
find_package(histograms REQUIRED)
target_link_libraries(my_target PRIVATE kressler::histograms)
```

**In code**:
```cpp
#include <histograms/histogram.h>
#include <histograms/log_linear_bucketer.h>
#include <histograms/linear_bucketer.h>
#include <histograms/log_bucketer.h>
```

### INTERFACE Library Pattern
- Header-only library uses CMake INTERFACE target
- Generator expressions for include directories:
  - `$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>`
  - `$<INSTALL_INTERFACE:include>`
- Namespace alias: `kressler::histograms`

## Important Technical Details

### LogLinearBucketer Algorithm
1. **Linear phase**: Values [0, kLinearThreshold) map 1:1 to buckets
   - `kLinearThreshold = (1 << (SignificantBits + 1)) * Scale`
2. **Log-linear phase**:
   - Find MSB position: `std::bit_width(value / Scale) - 1`
   - Major index: which power-of-2 range
   - Minor index: position within that range (top SignificantBits)
   - Result: `kLinearThreshold / Scale + (major << SignificantBits) + minor`

### LinearBucketer Algorithm
- Bucket calculation: `(value - Min) / Scale` when `value >= Min`
- Bucket boundaries: `Min + i * Scale` for bucket i
- Values < Min map to bucket 0
- Values beyond range clamp to `Buckets - 1`

### LogBucketer Algorithm
- Uses `std::bit_width(value)` for efficient log₂ calculation
- Special handling: value 0 → bucket 0, value 1 → bucket 1
- Bucket i (i ≥ 2): values `[2^(i-1), 2^i)`

### Percentile Calculation
1. Build cumulative counts array
2. For each percentile p, compute `target_count = p * total`
3. Find bucket where cumulative >= target_count
4. Linear interpolation between lower and upper bucket boundaries
5. Special cases:
   - p=0: Find first non-empty bucket
   - Last bucket: Return `std::numeric_limits<double>::max()`

## Common Tasks

### Add new bucketer type
1. Create header in `include/histograms/`
2. Implement `static size_t bucket(size_t value)`
3. Implement `static std::vector<size_t> bucket_boundaries()`
4. Add tests in `tests/`
5. Update install rules in root CMakeLists.txt
6. Update README.md with bucketer documentation

### Add histogram method
1. Update `include/histograms/histogram.h`
2. Add const correctness
3. Add comprehensive tests to `histogram_test.cc`
4. Document with examples

### Release checklist
1. Update version in CMakeLists.txt (write_basic_package_version_file)
2. Run all tests: `ctest --test-dir cmake-build-release`
3. Run clang-tidy: `cmake --build build --target clang-tidy`
4. Test installation to temporary location
5. Create git tag: `git tag -a v1.x.x -m "Release 1.x.x"`

## Key Learnings

### From PR History
- **LogLinearBucketer**: Added boundary tests, made variables const, removed zero checks
- **Histogram class**: Added include_empty param, removed bounds check in observe()
- **Percentiles**: Return max() for last bucket, use linear interpolation
- **CMake export**: INTERFACE library, package config, install rules
- **LinearBucketer & LogBucketer**: Added for general-purpose bucketing (simpler alternatives)
- **clang-tidy integration**: Enforced code quality standards

### Performance Notes
- Bucketing is `O(1)` using bit operations
- No virtual functions (templates resolve at compile time)
- No dynamic allocation in hot path (all bucket operations are constexpr/inline)
- AVX2 vectorization potential for future optimizations

## Gotchas

- **Catch2 submodule**: Must be initialized (`git submodule update --init --recursive`)
- **C++23 requirement**: Consumer projects must use C++23 or later
- **Scale parameter**: Applied in bucketer, affects bucket boundaries
- **Clamping**: Values beyond max bucket map to `(Buckets - 1)`, not overflow
- **Last bucket unbounded**: Histogram doesn't know upper limit, percentiles return max()
- **Pre-commit hook**: Blocks commit if clang-tidy warnings found (can bypass with `--no-verify`)
