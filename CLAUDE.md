# Claude's Working Notes for histograms Repository

## Project Overview
Header-only C++23 template library for high-performance histogram tracking with log-linear bucketing.

**Namespace**: `kressler::histograms`
**Standard**: C++23
**Style Guide**: Google C++ Style
**Testing**: Catch2 v3 (git submodule in `third_party/Catch2`)
**Optimizations**: AVX2, FMA, march=haswell, mtune=native

## Repository Structure
```
histograms/
├── src/
│   ├── histogram.h              # Histogram<Bucketer> template class
│   ├── log_linear_bucketer.h    # LogLinearBucketer<Buckets, SigBits, Scale>
│   └── tests/
│       ├── histogram_test.cc    # 16 test cases
│       └── log_linear_bucketer_test.cc  # 9 test cases
├── cmake/
│   └── histogramsConfig.cmake.in  # CMake package config
├── third_party/Catch2/          # Git submodule
└── CMakeLists.txt               # Root build config
```

## Core Components

### LogLinearBucketer
Template parameters: `<Buckets, SignificantBits, Scale>`
- Two phases: linear [0, 2^(SignificantBits+1) * Scale), then log-linear
- Uses bit manipulation (std::countl_zero, bit shifting)
- Values clamped to `(Buckets - 1)` at upper bound
- All intermediate variables are `const`
- Methods: `bucket(value)`, `bucket_boundaries()`

### Histogram
Template parameter: `<typename Bucketer>`
- Methods:
  - `observe(value, n=1)` - No bounds check, trusts bucketer
  - `data(include_empty=false)` - Returns vector of (boundary, count) pairs
  - `clear()` - Reset all counts
  - `total_count()` - Sum of all observations
  - `percentiles(vector<double>)` - Linear interpolation, returns infinity for last bucket

## Build & Test

### Build Directories
- **Debug builds**: `cmake-build-debug` (default configuration)
- **Release builds**: `cmake-build-release` (optimized)
- **ASAN builds**: `cmake-build-asan` (memory debugging)

### Debug Build
```bash
cmake -B cmake-build-debug
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug
```

### Release Build
```bash
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
ctest --test-dir cmake-build-release
```

### ASAN Build
```bash
cmake -B cmake-build-asan -DENABLE_ASAN=ON
cmake --build cmake-build-asan
ctest --test-dir cmake-build-asan
```

### Options
- `ENABLE_ASAN=ON` - AddressSanitizer for memory debugging
- `CMAKE_BUILD_TYPE=Release` - Aggressive optimizations (-O3, -ffast-math, -flto)

### Pre-commit Hook
Automatically formats .cpp/.hpp/.ipp files with clang-format. Located at `.git/hooks/pre-commit`.

### Format Target
```bash
cmake --build cmake-build-debug --target format
```

## Development Workflow

### Branch Naming
- `feature/descriptive-name` for new features
- Work on branches, create PRs to `main`

### Commit Style
```
Brief imperative description of change

More detailed explanation if needed.

Changes:
- Bullet points of specific changes

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

### PR Pattern
```markdown
## Summary
- 1-3 bullet points

## Changes
- Specific changes made

## Test plan
- [x] Tests added/updated
- [x] All tests pass
```

## Code Conventions

### Always
- Mark all intermediate variables `const` where possible
- Use `size_t` for indices, counts, and values
- Use `constexpr` for compile-time constants
- Prefer `noexcept` on performance-critical paths
- Include headers: algorithm, cstddef, vector, limits, utility, bit
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

## CMake Export

Library is installable for consumption by other projects:

```cmake
find_package(histograms REQUIRED)
target_link_libraries(my_target PRIVATE histograms::histograms)
```

Include in consumer code:
```cpp
#include <histograms/histogram.h>
#include <histograms/log_linear_bucketer.h>
```

Install destination: `lib/cmake/histograms/`, `include/histograms/`

## Important Technical Details

### LogLinearBucketer Algorithm
1. **Linear phase**: Values [0, kLinearThreshold) map 1:1 to buckets
   - `kLinearThreshold = 1 << (SignificantBits + 1)`
2. **Log-linear phase**:
   - Find MSB position: `63 - std::countl_zero(scaled)`
   - Major index: which power-of-2 range
   - Minor index: position within that range (top SignificantBits)
   - Result: `kLinearThreshold + (major << SignificantBits) + minor`

### Percentile Calculation
1. Build cumulative counts array
2. For each percentile p, compute `target_count = p * total`
3. Find bucket where cumulative >= target_count
4. Linear interpolation between lower and upper bucket boundaries
5. Special cases:
   - p=0: Find first non-empty bucket
   - Last bucket: Return `std::numeric_limits<double>::infinity()`

### INTERFACE Library Pattern
- Header-only library uses CMake INTERFACE target
- Generator expressions for include directories:
  - `$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>`
  - `$<INSTALL_INTERFACE:include>`
- Namespace alias: `histograms::histograms`

## PR History & Patterns

| PR | Feature | Key Learnings |
|----|---------|---------------|
| #2 | LogLinearBucketer | Added boundary tests, made variables const, removed zero checks |
| #4 | Histogram class | Added include_empty param, removed bounds check in observe() |
| #5 | Percentiles | Return infinity for last bucket, use linear interpolation |
| #6 | CMake export | INTERFACE library, package config, install rules |

## Common Tasks

### Add new bucketer type
1. Create header in `src/`
2. Implement `static size_t bucket(size_t value)`
3. Implement `static std::vector<size_t> bucket_boundaries()`
4. Add tests in `src/tests/`
5. Update install rules in root CMakeLists.txt

### Add histogram method
1. Update `src/histogram.h`
2. Add const correctness
3. Add comprehensive tests to `histogram_test.cc`
4. Document with examples

### Release checklist
1. Update version in CMakeLists.txt (write_basic_package_version_file)
2. Run all tests: `ctest --test-dir cmake-build-release`
3. Test installation to temporary location
4. Create git tag: `git tag -a v1.x.x -m "Release 1.x.x"`

## Gotchas

- **Catch2 submodule**: Must be initialized (`git submodule update --init --recursive`)
- **C++23 requirement**: Consumer projects must use C++23 or later
- **Scale parameter**: Applied in bucketer, affects bucket boundaries
- **Clamping**: Values beyond max bucket map to `(Buckets - 1)`, not overflow
- **Last bucket unbounded**: Histogram doesn't know upper limit, percentiles return infinity
- **No dynamic allocation in hot path**: All bucket operations are constexpr/inline

## Performance Notes

- Bucketing is `O(1)` using bit operations
- No virtual functions (templates resolve at compile time)
- AVX2 vectorization potential in future (buckets are contiguous)
- Consider `[[gnu::hot]]` attribute for observe() if profiling shows benefit
