#pragma once

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <random>
#include "multithreader_pool.h"
#include "logging_util.h"
#include "hash_map.h"
#include "spin_lock.h"
#include "phmap.h"
#include "phmap_specialized.h"
#include "paging_allocator.h"

#define PKLE_INCLUDE_ABSEIL_HASHMAP 1
#if PKLE_INCLUDE_ABSEIL_HASHMAP
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#endif

#define PKLE_INCLUDE_PARLAY_HASHMAP 0
#if PKLE_INCLUDE_PARLAY_HASHMAP
#include "parlay_hash/unordered_map.h"
#endif

// Benchmark result structure
struct HashmapBenchmarkResult
{
    const char* testName;
    uint64_t durationNs;
    uint64_t operationCount;
    double opsPerSecond;
    double avgLatencyNs;
    uint32_t threadCount;
    const char* operationType;

    void Print() const
    {
        printf("%-70s [%2d threads] [%s]: %10llu ns, %10llu ops, %12.2f ops/sec, %.2f ns/op\n",
               testName,
               threadCount,
               operationType,
               (unsigned long long)durationNs,
               (unsigned long long)operationCount,
               opsPerSecond,
               avgLatencyNs);
    }
};

// Key generation strategy
class KeyGenerator
{
inline static constexpr uint64_t MAX_RNG_KEY_NUMBER = 120000;

public:
    // Sequential keys (good for cache, predictable)
    static uint64_t Sequential(uint32_t threadId, uint32_t iteration, uint32_t totalThreads)
    {
        return threadId * 1000000ULL + iteration;
    }

    // Random keys (realistic workload)
    static uint64_t Random(uint32_t threadId, uint32_t iteration, uint32_t totalThreads)
    {
        // Use thread-local random generator
        static thread_local std::mt19937_64 rng(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        return rng() % MAX_RNG_KEY_NUMBER;
    }

    // Contended keys (high collision scenario)
    static uint64_t Contended(uint32_t threadId, uint32_t iteration, uint32_t totalThreads)
    {
        // Map to a smaller key space to create contention
        return iteration % 100;
    }

    // Strided keys (spread across threads)
    static uint64_t Strided(uint32_t threadId, uint32_t iteration, uint32_t totalThreads)
    {
        return threadId + (iteration * totalThreads);
    }

    template<typename FuncType>
    static const char* GetKeyGenName(const FuncType& func)
    {
        if (func == Sequential)
            return "Sequential";
        else if (func == Random)
            return "Random";
        else if (func == Contended)
            return "Contended";
        else if (func == Strided)
            return "Strided";
        return "Unknown";
    }

};

struct TestValueStruct
{
    uint64_t data[4] = {0};
    std::string info;
    std::vector<uint8_t> blob;

    TestValueStruct() = default;
    TestValueStruct(uint64_t val)
    {
        data[0] = val;
        info = "Value_" + std::to_string(val);
        blob.resize(64, static_cast<uint8_t>(val % 256));
    }
};

// Base class for hashmap benchmarks
class HashmapBenchmarkTest : public ::testing::Test
{
public:
    static constexpr uint32_t OPERATIONS_PER_THREAD = 100000;
    static constexpr uint32_t WORK_CYCLES = 10; // Simulated work
    static constexpr uint32_t PRELOAD_KEYS = 10000; // Keys to preload for read tests
    static constexpr uint32_t ITERATOR_OPERATIONS = 5;

protected:
    void SetUp() override
    {
        PklE::Util::Logging::InitLogging();
    }

    void TearDown() override
    {
        PklE::Util::Logging::ShutdownLogging();
    }

    // Helper to simulate some work
    static void SimulateWork()
    {
        volatile uint64_t dummy = 0;
        for (uint32_t i = 0; i < WORK_CYCLES; ++i)
        {
            dummy = dummy + i;
        }
    }

public:
    // Helper to create benchmark result
    static HashmapBenchmarkResult CreateResult(
        const char* name,
        std::chrono::nanoseconds duration,
        uint64_t operations,
        uint32_t threadCount,
        const char* operationType = "mixed")
    {
        HashmapBenchmarkResult result;
        result.testName = name;
        result.durationNs = duration.count();
        result.operationCount = operations;
        result.opsPerSecond = (operations * 1e9) / duration.count();
        result.avgLatencyNs = static_cast<double>(duration.count()) / operations;
        result.threadCount = threadCount;
        result.operationType = operationType;
        return result;
    }

    // Templated helper to run a test with a specific number of threads
    template<uint32_t NUM_THREADS>
    static void RunWithThreadCount(
        const char* testName,
        auto&& testLogic,
        uint64_t expectedCount,
        const char* operationType = "mixed")
    {
        using ThreadPoolType = PklE::MultiThreader::WorkerThreadPool<1, NUM_THREADS, 1, 500, 64>;
        ThreadPoolType pool;
        typename ThreadPoolType::ThreadDistribution distribution;
        distribution[0] = NUM_THREADS;
        typename ThreadPoolType::ThreadDistribution balanced = pool.DistributeThreads(distribution);
        pool.StartThreads(balanced);

        auto start = std::chrono::high_resolution_clock::now();

        const uint32_t c_maxTasks = 64;
        const uint32_t c_elementsPerTask = 25;
        const bool bCallingExternally = true;

        pool.template RunParallelForInRangeInQueue<c_maxTasks>(
            0, expectedCount, 0, testLogic, c_elementsPerTask, bCallingExternally);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        auto result = CreateResult(testName, duration, expectedCount, NUM_THREADS, operationType);
        result.Print();
    }

    // Run a benchmark across multiple thread counts
    template<typename HashmapType>
    static void RunThreadScalingBenchmark(
        const char* baseName,
        HashmapType& hashmap,
        auto&& setupFunc,
        auto&& testLogic,
        uint64_t expectedCount,
        const char* operationType = "mixed",
        bool bSingleThreadedOnly = false
    )
    {
        if(!bSingleThreadedOnly)
        {
            // Run with varying thread counts
            setupFunc(hashmap);
            RunWithThreadCount<16>(baseName, testLogic, expectedCount, operationType);
            
            setupFunc(hashmap);
            RunWithThreadCount<8>(baseName, testLogic, expectedCount, operationType);
            
            setupFunc(hashmap);
            RunWithThreadCount<4>(baseName, testLogic, expectedCount, operationType);
            
            setupFunc(hashmap);
            RunWithThreadCount<2>(baseName, testLogic, expectedCount, operationType);
            
            setupFunc(hashmap);
            RunWithThreadCount<1>(baseName, testLogic, expectedCount, operationType);
        }
        else
        {
            for(uint32_t i = 0; i < expectedCount; ++i)
            {
                // Single-threaded only
                setupFunc(hashmap);
                RunWithThreadCount<1>(baseName, testLogic, expectedCount, operationType);
            }
        }
    }

    // Preload hashmap with keys for read benchmarks
    template<typename HashmapType, typename KeyGenFunc>
    static void PreloadHashmap(HashmapType& hashmap, uint32_t keyCount, KeyGenFunc&& keyGen)
    {
        for (uint32_t i = 0; i < keyCount; ++i)
        {
            uint64_t key = keyGen(0, i, 1);
            hashmap.insert(key, key * 2); // Value = key * 2
        }
    }
};

// Test fixture for insert-heavy workloads
class HashmapInsertTest : public HashmapBenchmarkTest {};

// Test fixture for batch insert workloads
class HashmapBatchInsertTest : public HashmapBenchmarkTest {};

// Test fixture for lookup-heavy workloads
class HashmapLookupTest : public HashmapBenchmarkTest {};

// Test fixture for batched lookup workloads
class HashmapBatchedLookupTest : public HashmapBenchmarkTest {};

// Test fixture for erase-heavy workloads
class HashmapEraseTest : public HashmapBenchmarkTest {};

// Test fixture for mixed workloads
class HashmapMixedTest : public HashmapBenchmarkTest {};

// Test fixture for contended workloads
class HashmapContendedTest : public HashmapBenchmarkTest {};

// Test fixture for rekey workloads
class HashmapRekeyTest : public HashmapBenchmarkTest {};

// Test fixture for iterator workloads
class HashmapIteratorTest : public HashmapBenchmarkTest {};

// ============================================================================
// HASHMAP WRAPPER TEMPLATES
// These wrappers provide a consistent interface for different hashmap types
// Locking is handled externally via CountingSpinlock passed to test operations
// ============================================================================

// Wrapper for std::unordered_map (no internal locking)
template<typename KeyType, typename ValueType>
class StdUnorderedMapLocked
{
private:
    using MapType = std::unordered_map<KeyType, ValueType*, PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>>;
    using PoolType = PklE::CoreTypes::PagingObjectPool<ValueType, 8>;
    using LockType = PklE::CoreTypes::CountingSpinlock;
    
    PoolType pool_;
    MapType map_;
    mutable LockType spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "StdUnorderedMapLocked";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        bool bInserted = false;
        ValueType* pValue = pool_.Reserve(std::forward<Args>(args)...);
        if(pValue)
        {
            PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
            bInserted = map_.try_emplace(key, pValue).second;
        }

        if(!bInserted)
        {
            pool_.Release(pValue);
        }

        return bInserted;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        PklE::CoreTypes::ScopedReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = (it->second);
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        return map_.erase(key) > 0;
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            ValueType* value = it->second;
            map_.erase(it);
            map_.insert(std::make_pair(newKey, value));
            return true;
        }
        return false;
    }

    void reserve(size_t numElements)
    {
        map_.reserve(numElements);
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        // No difference for std::unordered_map since it has no internal locking
        return insert(key, std::forward<Args>(args)...);
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Do a find without locking for batched operations
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = (it->second);
            return true;
        }
        return false;
    }

    void clear()
    {
        map_.~MapType();
        new (&map_) MapType();
        pool_.Clear();
    }

    size_t size() const
    {
        return map_.size();
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, *(pair.second));
        }
    }
};

// Wrapper for PklE::ThreadsafeContainers::HashMap
template<typename KeyType, typename ValueType, bool UseLockless = false>
class PklEHashMap
{
private:
    inline static constexpr uint32_t c_pageSize = 8;
    inline static constexpr uint32_t c_numInnerMaps = (UseLockless) ? 1 : 2;
    using MapType = PklE::ThreadsafeContainers::HashMap<KeyType, ValueType, c_pageSize, c_numInnerMaps>;

    mutable PklE::CoreTypes::CountingSpinlock spinLock_;
    MapType map_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        if(UseLockless)
        {
            return "PklEHashMapLocked";
        }
        else
        {
            return "PklEHashMap";
        }
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        if(UseLockless)
        {
            PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
            return map_.Insert_Lockless(key, std::forward<Args>(args)...);
        }
        else
        {
            return map_.Insert_Concurrent(key, std::forward<Args>(args)...);
        }
        return false;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        if(UseLockless)
        {
            PklE::CoreTypes::ScopedReadSpinLock lock(spinLock_);
            return map_.find_lockless(key, outValue);
        }
        return map_.find(key, outValue);
    }

    bool erase(const KeyType& key)
    {
        if(UseLockless)
        {
            PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
            return map_.erase_lockless(key);
        }
        return map_.erase(key);
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        if(UseLockless)
        {
            PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
            return map_.rekey_lockless(oldKey, newKey);
        }
        return map_.rekey(oldKey, newKey);
    }

    void reserve(size_t numElements)
    {
        map_.reserve(numElements);
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        if(UseLockless)
        {
            PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
            return map_.Insert_Lockless(key, std::forward<Args>(args)...);
        }
        return map_.Insert_Concurrent(key, std::forward<Args>(args)...);
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        return map_.find_lockless(key, outValue);
    }

    void clear()
    {
        map_.~MapType();
        new (&map_) MapType();
    }

    size_t size() const
    {
        return map_.size();
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for(auto& pair : map_)
        {
            callback(pair.key, pair.value);
        }
    }
};

// ============================================================================
// PHMAP SPINLOCK WRAPPERS
// 
// The phmap_specialized.h provides phmap variants using custom CountingSpinlock:
// 
// 1. PhmapParallelFlatHashMapSpinlock:
//    - Uses parallel_flat_hash_map_spinlock (standard read-write lock)
//    - Has built-in internal locking using CountingSpinlock
//    - Thread-safe by default, no external locking needed
//    - Reader priority: readers don't block other readers
//    - Template parameter N controls number of submaps (default 4)
// 
// 2. PhmapParallelFlatHashMapWritePriority:
//    - Uses parallel_flat_hash_map_write_priority
//    - Write-priority locking: writers get priority over readers
//    - Useful for write-heavy workloads to prevent writer starvation
//    - Template parameter N controls number of submaps (default 4)
// 
// All wrappers support pointer-based values for integration with PagingObjectPool.
// ============================================================================

// Wrapper for parallel_flat_hash_map_spinlock (standard read-write lock)
template<typename KeyType, typename ValueType, size_t N = 4>
class PhmapParallelFlatHashMapSpinlock
{
private:
    using MapType = PklE::ThreadsafeContainers::parallel_flat_hash_map_spinlock<
        KeyType, 
        ValueType*,
        PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>,
        std::equal_to<KeyType>,
        std::allocator<std::pair<const KeyType, ValueType*>>,
        N>;

    using PoolType = PklE::CoreTypes::PagingObjectPool<ValueType, 8>;
    
    PoolType pool_;
    MapType map_;

    mutable PklE::CoreTypes::CountingSpinlock spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "PhmapParallelFlatHashMapSpinlock";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        bool bInserted = false;
        ValueType* pValue = pool_.Reserve(std::forward<Args>(args)...);
        if(pValue)
        {
            PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
            bInserted = map_.try_emplace(key, pValue).second;
        }

        if(!bInserted)
        {
            pool_.Release(pValue);
        }

        return bInserted;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        // Need to lock because an insert could cause a resize which invalidates iterators
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = it->second;
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
        return map_.erase(key) > 0;
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        // Need to lock because an insert could cause a resize which invalidates iterators
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock writeLock(std::move(lock));
            ValueType* value = it->second;
            map_.erase(it);
            map_.try_emplace(newKey, value);
            return true;
        }
        return false;
    }
    
    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        bool bInserted = false;

        // Parallel flat hash map has internal locking, so we can just insert directly
        ValueType* pValue = pool_.Reserve(std::forward<Args>(args)...);
        if(pValue)
        {
            bInserted = map_.try_emplace(key, pValue).second;
            if(!bInserted)
            {
                pool_.Release(pValue);
            }
        }

        return bInserted;
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Parallel flat hash map has internal locking, so we can just find directly
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = it->second;
            return true;
        }
        return false;
    }

    void clear()
    {
        map_.~MapType();
        new (&map_) MapType();
        pool_.Clear();
    }


    size_t size() const
    {
        return map_.size();
    }

    void reserve(size_t count)
    {
        map_.reserve(count);
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, *pair.second);
        }
    }
};


#if PKLE_INCLUDE_ABSEIL_HASHMAP
// ============================================================================
// ABSEIL WRAPPERS
// 
// The Abseil container library provides highly optimized hashmap implementations:
// 
// 1. AbseilFlatHashMapLocked:
//    - Uses absl::flat_hash_map (single-threaded, no internal locking)
//    - Requires external locking for thread-safety
//    - Based on Swiss Tables algorithm - very fast and memory efficient
//    - Best for single-threaded use or when you have custom locking strategy
//    - Better performance than std::unordered_map in most cases
// 
// All wrappers support pointer-based values for integration with PagingObjectPool.
// ============================================================================

// Wrapper for absl::flat_hash_map (single-threaded, no internal locking)
template<typename KeyType, typename ValueType>
class AbseilFlatHashMapLocked
{
private:
    using MapType = absl::flat_hash_map<KeyType, ValueType*, PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>>;
    using PoolType = PklE::CoreTypes::PagingObjectPool<ValueType, 8>;
    using LockType = PklE::CoreTypes::CountingSpinlock;


    MapType map_;
    PoolType pool_;
    mutable LockType spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "AbseilFlatHashMapLocked";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        bool bInserted = false;
        ValueType* pValue = pool_.Reserve(std::forward<Args>(args)...);
        if(pValue)
        {
            PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
            bInserted = map_.try_emplace(key, pValue).second;
        }

        if(!bInserted)
        {
            pool_.Release(pValue);
        }

        return bInserted;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        PklE::CoreTypes::ScopedReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = it->second;
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        return map_.erase(key) > 0;
    }

    void clear()
    {
        map_.~MapType();
        new (&map_) MapType();
        pool_.Clear();
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            ValueType* value = it->second;
            map_.erase(it);
            map_.try_emplace(newKey, value);
            return true;
        }
        return false;
    }

    void reserve(size_t numElements)
    {
        map_.reserve(numElements);
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        // No difference for absl::flat_hash_map since it has no internal locking
        return insert(key, std::forward<Args>(args)...);
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Do a find without locking for batched operations
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = it->second;
            return true;
        }
        return false;
    }

    size_t size() const
    {
        return map_.size();
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, *pair.second);
        }
    }
};

// Wrapper for absl::node_hash_map (single-threaded, no internal locking, with pointer stability)
template<typename KeyType, typename ValueType>
class AbseilNodeHashMapLocked
{
private:
    using MapType = absl::node_hash_map<KeyType, ValueType, PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>>;

    MapType map_;
    mutable PklE::CoreTypes::CountingSpinlock spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "AbseilNodeHashMapLocked";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        auto result = map_.try_emplace(key, std::forward<Args>(args)...);
        return result.second;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        PklE::CoreTypes::ScopedReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &it->second;
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        return map_.erase(key) > 0;
    }

    void clear()
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        map_.clear();
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            ValueType value = std::move(it->second);
            map_.erase(it);
            map_.try_emplace(newKey, std::move(value));
            return true;
        }
        return false;
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        // No difference for absl::node_hash_map since it has no internal locking
        return insert(key, std::forward<Args>(args)...);
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Do a find without locking for batched operations
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &it->second;
            return true;
        }
        return false;
    }

    size_t size() const
    {
        return map_.size();
    }

    void reserve(size_t count)
    {
        map_.reserve(count);
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, pair.second);
        }
    }
};

// Wrapper for absl::node_hash_map with StdPagingAllocator (pointer stability + efficient allocation)
template<typename KeyType, typename ValueType>
class AbseilNodeHashMapPagingAllocator
{
private:
    using MapType = absl::node_hash_map<
        KeyType, 
        ValueType,
        PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>,
        std::equal_to<KeyType>,
        PklE::CoreTypes::StdPagingAllocator<std::pair<const KeyType, ValueType>>>;

    MapType map_;
    mutable PklE::CoreTypes::CountingSpinlock spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "AbseilNodeHashMapPagingAllocator";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        auto result = map_.try_emplace(key, std::forward<Args>(args)...);
        return result.second;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        PklE::CoreTypes::ScopedReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &it->second;
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        return map_.erase(key) > 0;
    }

    void clear()
    {
        // Call the destructor to make sure all memory is freed
        map_.~MapType();

        // Clearing out the allocator between tests to keep results consistent
        PklE::CoreTypes::StdPagingAllocator<std::pair<const KeyType, ValueType>>::ClearShared();

        // Reconstruct the map
        new(&map_) MapType();
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        PklE::CoreTypes::ScopedWriteSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            ValueType value = std::move(it->second);
            map_.erase(it);
            map_.try_emplace(newKey, std::move(value));
            return true;
        }
        return false;
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        // No difference for absl::node_hash_map since it has no internal locking
        return insert(key, std::forward<Args>(args)...);
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Do a find without locking for batched operations
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &it->second;
            return true;
        }
        return false;
    }

    size_t size() const
    {
        return map_.size();
    }

    void reserve(size_t count)
    {
        map_.reserve(count);
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, pair.second);
        }
    }
};
#endif //PKLE_INCLUDE_ABSEIL_HASHMAP

#if PKLE_INCLUDE_PARLAY_HASHMAP
// ============================================================================
// PARLAY HASH WRAPPERS
// 
// The parlay_hash library provides a highly scalable hashmap implementation:
// 
// 1. ParlayUnorderedMapLocked:
//    - Uses parlay::parlay_unordered_map (designed for high concurrency)
//    - Requires external locking for thread-safety in this wrapper
//    - Designed for scalability to large number of threads and high contention
//    - Automatically selects direct or indirect storage based on type traits
//    - Better performance under high contention scenarios
// 
// All wrappers support pointer-based values for integration with PagingObjectPool.
// ============================================================================

// Wrapper for parlay::parlay_unordered_map (external locking)
template<typename KeyType, typename ValueType>
class ParlayUnorderedMapLocked
{
private:
    using MapType = parlay::parlay_unordered_map<KeyType, ValueType*>;
    using PoolType = PklE::CoreTypes::PagingObjectPool<ValueType, 8>;
    using LockType = PklE::CoreTypes::CountingSpinlock;

    mutable MapType map_;
    PoolType pool_;
    mutable LockType spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "ParlayUnorderedMap";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        bool bInserted = false;
        ValueType* pValue = pool_.Reserve(std::forward<Args>(args)...);
        if(pValue)
        {
            PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
            auto result = map_.Insert(key, pValue);
            bInserted = !result.has_value();
            if(!bInserted)
            {
                pool_.Release(pValue);
            }
        }
        return bInserted;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        // Need to lock because an insert could cause a resize which invalidates iterators
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto result = map_.Find(key);
        if (result.has_value())
        {
            outValue = *result;
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
        auto result = map_.Remove(key);
        if (result.has_value())
        {
            pool_.Release(*result);
            return true;
        }
        return false;
    }

    void clear()
    {
        map_.~MapType();
        new (&map_) MapType();
        pool_.Clear();
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        // Need to lock because an insert could cause a resize which invalidates iterators
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto it = map_.Find(oldKey);
        if (it.has_value())
        {
            PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock writeLock(std::move(lock));
            ValueType* value = *it;
            map_.Remove(oldKey);
            auto result = map_.Insert(newKey, value);
            if (result.has_value())
            {
                // New key already existed, restore old key
                map_.Insert(oldKey, value);
                return false;
            }
            return true;
        }
        return false;
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        bool bInserted = false;

        // Parlay hash map has internal locking, so we can just insert directly
        ValueType* pValue = pool_.Reserve(std::forward<Args>(args)...);
        if(pValue)
        {
            auto result = map_.Insert(key, pValue);
            bInserted = !result.has_value();
            if(!bInserted)
            {
                pool_.Release(pValue);
            }
        }

        return bInserted;
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Parlay hash map has internal locking, so we can just find directly
        auto result = map_.Find(key);
        if (result.has_value())
        {
            outValue = *result;
            return true;
        }
        return false;
    }

    size_t size() const
    {
        return map_.size();
    }

    void reserve(size_t count)
    {
        // Parlay hash map grows automatically, no reserve needed
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, *(pair.second));
        }
    }
};
#endif //PKLE_INCLUDE_PARLAY_HASHMAP

// ============================================================================
// PHMAP NODE HASH MAP WRAPPERS WITH PAGING OBJECT POOL ALLOCATOR
// 
// The phmap_specialized.h provides node hash map variants with custom allocator:
// 
// 1. PhmapParallelNodeHashMapSpinlock:
//    - Uses parallel_node_hash_map_spinlock with PagingObjectPoolAllocator
//    - Has built-in internal locking using CountingSpinlock
//    - Thread-safe by default, no external locking needed
//    - Reader priority: readers don't block other readers
//    - Uses PagingObjectPoolAllocator for efficient node allocation
//    - Template parameter N controls number of submaps (default 4)
// 
// 2. PhmapParallelNodeHashMapWritePriority:
//    - Uses parallel_node_hash_map_write_priority with PagingObjectPoolAllocator
//    - Write-priority locking: writers get priority over readers
//    - Useful for write-heavy workloads to prevent writer starvation
//    - Uses PagingObjectPoolAllocator for efficient node allocation
//    - Template parameter N controls number of submaps (default 4)
// 
// These wrappers support VALUE-based storage (not pointers) because the
// allocator manages the node allocation internally.
// ============================================================================

// Wrapper for parallel_node_hash_map_spinlock with PagingObjectPoolAllocator
template<typename KeyType, typename ValueType, size_t N = 4>
class PhmapParallelNodeHashMapSpinlock
{
private:
    using MapType = PklE::ThreadsafeContainers::parallel_node_hash_map_spinlock<
        KeyType, 
        ValueType,
        PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>,
        std::equal_to<KeyType>,
        std::allocator<std::pair<const KeyType, ValueType>>,
        N>;

        MapType map_;
        mutable PklE::CoreTypes::CountingSpinlock spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "PhmapParallelNodeHashMapSpinlock";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
        return map_.insert(std::make_pair(key, ValueType(std::forward<Args>(args)...))).second;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &(it->second);
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        return map_.erase(key) > 0;
    }

    void clear()
    {
        map_.~MapType();
        new (&map_) MapType();
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock writeLock(std::move(lock));
            ValueType value = it->second;
            map_.erase(it);
            map_.insert(std::make_pair(newKey, value));
            return true;
        }
        return false;
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        // Parallel flat hash map has internal locking, so we can just insert directly
        return map_.insert(std::make_pair(key, ValueType(std::forward<Args>(args)...))).second;
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Parallel flat hash map has internal locking, so we can just find directly
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &(it->second);
            return true;
        }
        return false;
    }

    size_t size() const
    {
        return map_.size();
    }

    void reserve(size_t count)
    {
        map_.reserve(count);
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, pair.second);
        }
    }
};


// ============================================================================
// PHMAP NODE HASH MAP WITH PAGING ALLOCATOR
// 
// This wrapper uses phmap's parallel_node_hash_map_spinlock with StdPagingAllocator
// from paging_allocator.h instead of std::allocator for efficient memory management.
// 
// - Uses parallel_node_hash_map_spinlock with StdPagingAllocator
// - Has built-in internal locking using CountingSpinlock
// - Thread-safe by default, no external locking needed
// - Reader priority: readers don't block other readers
// - Uses StdPagingAllocator for efficient node allocation
// - Template parameter N controls number of submaps (default 4)
// 
// Supports VALUE-based storage (not pointers) as the allocator manages nodes.
// ============================================================================

// Wrapper for parallel_node_hash_map_spinlock with StdPagingAllocator
template<typename KeyType, typename ValueType, size_t N = 4>
class PhmapParallelNodeHashMapPagingAllocator
{
private:
    using MapType = PklE::ThreadsafeContainers::parallel_node_hash_map_spinlock<
        KeyType, 
        ValueType,
        PklE::ThreadsafeContainers::PklEHashAdapter<KeyType>,
        std::equal_to<KeyType>,
        PklE::CoreTypes::StdPagingAllocator<std::pair<const KeyType, ValueType>>,
        N>;
    MapType map_;
    mutable PklE::CoreTypes::CountingSpinlock spinLock_;

public:
    using HashMapValueType = ValueType;

    static const char* GetMapTypeName()
    {
        return "PhmapParallelNodeHashMapPagingAllocator";
    }

    template<typename... Args>
    bool insert(const KeyType& key, Args&&... args)
    {
        PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
        return map_.insert(std::make_pair(key, ValueType(std::forward<Args>(args)...))).second;
    }

    bool find(const KeyType& key, const ValueType*& outValue) const
    {
        // Need to lock because an insert could cause a resize which invalidates iterators
        PklE::CoreTypes::ScopedMultiReaderWriterReadSpinLock lock(spinLock_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &(it->second);
            return true;
        }
        return false;
    }

    bool erase(const KeyType& key)
    {
        PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
        return map_.erase(key) > 0;
    }

    bool rekey(const KeyType& oldKey, const KeyType& newKey)
    {
        // Need to lock because an insert could cause a resize which invalidates iterators
        PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock lock(spinLock_);
        auto it = map_.find(oldKey);
        if (it != map_.end())
        {
            PklE::CoreTypes::ScopedMultiReaderWriterWriteSpinLock writeLock(std::move(lock));
            ValueType value = it->second;
            map_.erase(it);
            map_.insert(std::make_pair(newKey, value));
            return true;
        }
        return false;
    }

    template<typename... Args>
    bool insert_batched(const KeyType& key, Args&&... args)
    {
        // Parallel flat hash map has internal locking, so we can just insert directly
        return map_.insert(std::make_pair(key, ValueType(std::forward<Args>(args)...))).second;
    }

    bool find_batched(const KeyType& key, const ValueType*& outValue) const
    {
        // Parallel flat hash map has internal locking, so we can just find directly
        auto it = map_.find(key);
        if (it != map_.end())
        {
            outValue = &(it->second);
            return true;
        }
        return false;
    }

    void clear()
    {
        //Call the destructor to make sure all memory is freed
        map_.~MapType();

        //Clearing out the allocator between tests to keep results consistent
        PklE::CoreTypes::StdPagingAllocator<std::pair<const KeyType, ValueType>>::ClearShared();

        //Reconstruct the map
        new(&map_) MapType();
    }

    size_t size() const
    {
        return map_.size();
    }

    void reserve(size_t count)
    {
        map_.reserve(count);
    }

    template<typename CallbackType_T>
    void for_each(CallbackType_T&& callback)
    {
        for (auto& pair : map_)
        {
            callback(pair.first, pair.second);
        }
    }
};


// ============================================================================
// OPERATION TEMPLATES
// These templates provide common operation patterns
// ============================================================================

// Pure insert operation
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateInsertOperation(HashmapType& hashmap, KeyGenFunc&& keyGen, uint32_t threadCount)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        hashmap.insert(key, key * 2);
    };
}

// Batched insert operation
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateBatchedInsertOperation(HashmapType& hashmap, KeyGenFunc&& keyGen, uint32_t threadCount)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        hashmap.insert_batched(key, key * 2);
    };
}

// Pure lookup operation
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateLookupOperation(HashmapType& hashmap, KeyGenFunc&& keyGen, uint32_t threadCount, std::atomic<uint64_t>& successCounter)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount, &successCounter](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        const ValueType* pValue = nullptr;
        
        if (hashmap.find(key, pValue))
        {
            successCounter.fetch_add(1, std::memory_order_relaxed);
        }
    };
}

// Batched lookup operation
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateBatchedLookupOperation(HashmapType& hashmap, KeyGenFunc&& keyGen, uint32_t threadCount, std::atomic<uint64_t>& successCounter)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount, &successCounter](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        const ValueType* pValue = nullptr;

        if (hashmap.find_batched(key, pValue))
        {
            successCounter.fetch_add(1, std::memory_order_relaxed);
        }
    };
}

// Pure erase operation
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateEraseOperation(HashmapType& hashmap, KeyGenFunc&& keyGen, uint32_t threadCount, std::atomic<uint64_t>& successCounter)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount, &successCounter](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        
        if (hashmap.erase(key))
        {
            successCounter.fetch_add(1, std::memory_order_relaxed);
        }
    };
}

// Mixed operation (configurable read/write ratio)
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateMixedOperation(
    HashmapType& hashmap,
    KeyGenFunc&& keyGen,
    uint32_t threadCount,
    std::atomic<uint64_t>& readCounter,
    std::atomic<uint64_t>& writeCounter,
    uint32_t readPercentage = 90 // Default 90% reads, 10% writes
    )
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount, &readCounter, &writeCounter, readPercentage](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        
        // Determine operation type based on percentage
        bool isRead = (index % 100) < readPercentage;
        
        if (isRead)
        {
            const ValueType* pValue = nullptr;
            if (hashmap.find(key, pValue))
            {
                readCounter.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else
        {
            bool bInserted = hashmap.insert(key, key * 2);
            if(bInserted)
            {
                writeCounter.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };
}

// Complex mixed operation (insert/lookup/erase with configurable ratios)
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateComplexMixedOperation(
    HashmapType& hashmap,
    KeyGenFunc&& keyGen,
    uint32_t threadCount,
    std::atomic<uint64_t>& insertCounter,
    std::atomic<uint64_t>& lookupCounter,
    std::atomic<uint64_t>& eraseCounter,
    uint32_t insertPercentage = 40,
    uint32_t lookupPercentage = 50,
    uint32_t erasePercentage = 10)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount, 
            &insertCounter, &lookupCounter, &eraseCounter,
            insertPercentage, lookupPercentage, erasePercentage](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType key = keyGen(threadId, index, threadCount);
        
        uint32_t opSelector = index % 100;
        
        if (opSelector < insertPercentage)
        {
            bool bInserted = hashmap.insert(key, key * 2);
            if(bInserted)
            {
                insertCounter.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else if (opSelector < (insertPercentage + lookupPercentage))
        {
            const ValueType* pValue = nullptr;
            if (hashmap.find(key, pValue))
            {
                lookupCounter.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else
        {
            if (hashmap.erase(key))
            {
                eraseCounter.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };
}

// Rekey operation - changes keys of existing nodes
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
auto CreateRekeyOperation(
    HashmapType& hashmap,
    KeyGenFunc&& keyGen,
    uint32_t threadCount,
    std::atomic<uint64_t>& successCounter,
    uint64_t keyOffset)
{
    return [&hashmap, keyGen = std::forward<KeyGenFunc>(keyGen), threadCount, &successCounter, keyOffset](uint32_t index)
    {
        uint32_t threadId = index % threadCount;
        KeyType oldKey = keyGen(threadId, index, threadCount);
        KeyType newKey = oldKey + keyOffset;
        
        if (hashmap.rekey(oldKey, newKey))
        {
            successCounter.fetch_add(1, std::memory_order_relaxed);
        }
    };
}


template<typename KeyType, typename ValueType, typename HashmapType>
auto CreateIteratorOperation(
    HashmapType& hashmap,
    std::atomic<uint64_t>& iterationCounter)
{
    return [&hashmap, &iterationCounter](uint32_t /*index*/)
    {
        hashmap.for_each([&iterationCounter](const KeyType& key, const ValueType& value)
        {
            iterationCounter.fetch_add(1, std::memory_order_relaxed);
        });
    };
}
