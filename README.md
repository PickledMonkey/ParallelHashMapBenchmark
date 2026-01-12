# Parallel HashMap Benchmark Reference

This repository contains reference code and benchmark results from comprehensive testing of various parallel hashmap implementations in C++. This is **not runnable code** - it's intended as reference material for developers researching high-performance concurrent data structures.

> **📖 Full Article**: For detailed analysis and results, see the article at [pggame.dev]([https://pggame.dev](https://pggame.dev/2026/01/12/making-a-parallel-hash-map-should-you-even-try/))

## Intended Use

The best way to utilize this repository is to **point an AI agent at it** to extract specific implementation details, patterns, or benchmark results relevant to your use case. The code demonstrates various approaches to thread-safe hashmaps with different trade-offs.

## Test Environment

All benchmarks were performed under the following conditions:
- **Hardware**: 16-core GitHub Codespaces machine
- **Compiler**: Clang 18.1.3
- **Optimization Level**: `-O3`
- **Parallelism**: Thread scaling from 1 to 16 threads

## Repository Structure

### 📂 `src/custom_hashmap/`
Custom parallel hashmap implementation and supporting utilities:

- **[hash_map.h](src/custom_hashmap/hash_map.h)** - Main custom parallel hashmap implementation (PklE HashMap)
  - Template-based, lock-free where possible
  - Configurable inner map sharding
  - ~900 lines of core implementation
  
- **[spin_lock.h](src/custom_hashmap/spin_lock.h)** / **[spin_lock.cpp](src/custom_hashmap/spin_lock.cpp)** - Custom spinlock implementation

- **[paging_object_pool.h](src/custom_hashmap/paging_object_pool.h)** - Memory pool allocator with paging support

- **[fixedsize_object_pool.h](src/custom_hashmap/fixedsize_object_pool.h)** - Fixed-size object pool allocator

- **[simple_linked_list.h](src/custom_hashmap/simple_linked_list.h)** - Lock-free linked list for collision chains

- **[atomic_util.h](src/custom_hashmap/atomic_util.h)** - Atomic operation utilities

- **[magic_num_util.h](src/custom_hashmap/magic_num_util.h)** - Numeric utilities and bit manipulation helpers

- **[phmap_specialized.h](src/custom_hashmap/phmap_specialized.h)** - Specialized wrappers for parallel-hashmap library

### 📂 `src/benchmark/`
Benchmark framework and test harnesses:

- **[hashmap_benchmark.cpp](src/benchmark/hashmap_benchmark.cpp)** - Main benchmark implementation (~2000 lines)
  - Test harness for all operations
  - Thread scaling tests
  - Statistical analysis
  
- **[hashmap_benchmark.h](src/benchmark/hashmap_benchmark.h)** - Benchmark definitions and hashmap wrappers
  - Adapter classes for tested implementations
  - Operation generators
  - Result structures

### 📂 `data/`
Benchmark results and performance visualizations:

- **summary_statistics.txt** - Comprehensive statistics across all tests
- **PNG diagrams** - Latency and throughput charts for:
  - Individual operations (insert, lookup, erase, etc.)
  - Mixed workloads (40% insert / 50% lookup / 10% erase)
  - Read-heavy workloads (90% read / 10% write, 50% read / 50% write)
  - Sequential vs. random access patterns
  - Small vs. large value types

## Tested Hashmap Implementations

### Custom Implementation
- **PklEHashMap** - Custom parallel hashmap (main focus)
- **PklEHashMapLocked** - Variant of PklEHashMap where only the lockless operations were used, but had an external lock.

### Third-Party Implementations
- **StdUnorderedMapLocked** - Standard library `std::unordered_map` with mutex
- **AbseilFlatHashMapLocked** - Google Abseil flat hashmap with mutex
- **AbseilNodeHashMapLocked** - Google Abseil node hashmap with mutex
- **AbseilNodeHashMapPagingAllocator** - Abseil node map with custom allocator
- **PhmapParallelFlatHashMapSpinlock** - parallel-hashmap with spinlock
- **PhmapParallelNodeHashMapSpinlock** - parallel-hashmap node variant with spinlock
- **PhmapParallelNodeHashMapPagingAllocator** - parallel-hashmap with paging allocator

## Benchmark Operations

The test suite covers diverse concurrent scenarios:

### Basic Operations
- **insert** - inserts that are threadsafe with all other operations
- **batchInsert** - inserts that are threadsafe only with other inserts
- **lookup** - lookups that are threadsafe with all other operations
- **batchedLookup** - pure lookup testing with no locking
- **erase** - Key deletion
- **iterator** - Full map iteration

### Workload Patterns
- **40i50l10e** - 40% insert, 50% lookup, 10% erase (mixed workload)
- **50r50w** - 50% read, 50% write
- **90r10w** - 90% read, 10% write (read-heavy)
- **contendedInsert** - High contention on same keys
- **rekey** - Key modification operations

### Access Patterns
- **Sequential** - Predictable key sequences
- **Random** - Randomized keys

### Value Sizes
- **Small values** - `uint64_t` (8 bytes)
- **BigValue** - Larger structures (testing cache effects)

## Key Findings (Summary)

From `data/summary_statistics.txt`:

### Best Overall Performance (16 threads)
- **Batched Lookup**: PklEHashMapLocked - 608M ops/sec
- **Mixed Workloads**: PhmapParallelNodeHashMapSpinlock - 6.4-6.6M ops/sec
- **Read-Heavy**: PklEHashMapLocked - 7.4M ops/sec

### Average Performance (All Tests)
Best average performers:
1. PhmapParallelNodeHashMapSpinlock - 80.47 ns/op
2. PklEHashMapLocked - 84.51 ns/op
3. PklEHashMap - 120.60 ns/op

## Dependencies

The benchmark code uses:
- **Google Test** - Test framework
- **parallel-hashmap (phmap)** - High-performance hashmap library
- **Abseil** - Google's C++ common libraries
- Standard C++17 or later

## License

See [LICENSE](LICENSE) file for details.

---

**Note**: This code is provided as-is for reference purposes. It is not maintained as a library or production-ready package.
