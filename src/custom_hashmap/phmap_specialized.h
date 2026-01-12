#pragma once

// ---------------------------------------------------------------------------
// Specialized phmap containers using PklE::CoreTypes::CountingSpinlock
// ---------------------------------------------------------------------------

#include "spin_lock.h"
#include "phmap.h"
#include "hash_type.h"

namespace PklE
{
namespace ThreadsafeContainers
{
    // -----------------------------------------------------------------------
    // SpinlockMutexAdapter
    // -----------------------------------------------------------------------
    // Adapter class that makes CountingSpinlock compatible with phmap's 
    // expected mutex interface (which follows std::shared_mutex interface).
    // 
    // phmap expects:
    // - lock() / unlock() for exclusive (write) access
    // - lock_shared() / unlock_shared() for shared (read) access  
    // - try_lock() / try_lock_shared() for non-blocking variants
    // -----------------------------------------------------------------------
    class SpinlockMutexAdapter
    {
    public:
        SpinlockMutexAdapter() = default;
        ~SpinlockMutexAdapter() = default;

        // Non-copyable and non-movable (phmap expects mutex to be in-place)
        SpinlockMutexAdapter(const SpinlockMutexAdapter&) = delete;
        SpinlockMutexAdapter& operator=(const SpinlockMutexAdapter&) = delete;
        SpinlockMutexAdapter(SpinlockMutexAdapter&&) = delete;
        SpinlockMutexAdapter& operator=(SpinlockMutexAdapter&&) = delete;

        // Exclusive (write) lock interface
        void lock() 
        { 
            m_spinlock.AcquireReadAndWriteAccess(); 
        }

        void unlock() 
        { 
            m_spinlock.ReleaseReadAndWriteAccess(); 
        }

        bool try_lock() 
        { 
            // CountingSpinlock doesn't have a try_lock variant, so we just lock
            // This means it will spin until acquired
            lock();
            return true;
        }

        // Shared (read) lock interface
        void lock_shared() 
        { 
            m_spinlock.AcquireReadOnlyAccess(); 
        }

        void unlock_shared() 
        { 
            m_spinlock.ReleaseReadOnlyAccess(); 
        }

        bool try_lock_shared() 
        { 
            // CountingSpinlock doesn't have a try_lock variant, so we just lock
            // This means it will spin until acquired
            lock_shared();
            return true;
        }

    private:
        CoreTypes::CountingSpinlock m_spinlock;
    };

    // -----------------------------------------------------------------------
    // SpinlockWritePriorityMutexAdapter
    // -----------------------------------------------------------------------
    // Adapter class using write-priority locking semantics
    // -----------------------------------------------------------------------
    class SpinlockWritePriorityMutexAdapter
    {
    public:
        SpinlockWritePriorityMutexAdapter() = default;
        ~SpinlockWritePriorityMutexAdapter() = default;

        // Non-copyable and non-movable
        SpinlockWritePriorityMutexAdapter(const SpinlockWritePriorityMutexAdapter&) = delete;
        SpinlockWritePriorityMutexAdapter& operator=(const SpinlockWritePriorityMutexAdapter&) = delete;
        SpinlockWritePriorityMutexAdapter(SpinlockWritePriorityMutexAdapter&&) = delete;
        SpinlockWritePriorityMutexAdapter& operator=(SpinlockWritePriorityMutexAdapter&&) = delete;

        // Exclusive (write) lock interface
        void lock() 
        { 
            m_spinlock.AcquireWritePriorityReadAndWriteAccess(); 
        }

        void unlock() 
        { 
            m_spinlock.ReleaseWritePriorityReadAndWriteAccess(); 
        }

        bool try_lock() 
        { 
            lock();
            return true;
        }

        // Shared (read) lock interface
        void lock_shared() 
        { 
            m_spinlock.AcquireWritePriorityReadOnlyAccess(); 
        }

        void unlock_shared() 
        { 
            m_spinlock.ReleaseWritePriorityReadOnlyAccess(); 
        }

        bool try_lock_shared() 
        { 
            lock_shared();
            return true;
        }

    private:
        CoreTypes::CountingSpinlock m_spinlock;
    };


    // -----------------------------------------------------------------------
    // PklEHashAdapter
    // -----------------------------------------------------------------------
    // Adapter class that wraps PklE::Util::HashType::Hash64 to make it
    // compatible with phmap's hash interface expectations.
    // 
    // phmap expects a hash functor with:
    // - size_t operator()(const T&) const
    // - Optional: result_type and argument_type typedefs
    // -----------------------------------------------------------------------
    template <typename T>
    struct PklEHashAdapter
    {
        using result_type = size_t;
        using argument_type = T;

        size_t operator()(const T& value) const noexcept
        {
            return static_cast<size_t>(Util::HashType::Hash64(value));
        }
    };


    // -----------------------------------------------------------------------
    // Type aliases for phmap containers using SpinlockMutexAdapter
    // -----------------------------------------------------------------------

    // Standard read-write lock variants (default N=4 means 16 submaps)
    template <class K, class V,
              class Hash  = PklEHashAdapter<K>,
              class Eq    = phmap::priv::hash_default_eq<K>,
              class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const K, V>>,
              size_t N    = 4>
    using parallel_flat_hash_map_spinlock = 
        phmap::parallel_flat_hash_map<K, V, Hash, Eq, Alloc, N, SpinlockMutexAdapter>;

    template <class T,
              class Hash  = PklEHashAdapter<T>,
              class Eq    = phmap::priv::hash_default_eq<T>,
              class Alloc = phmap::priv::Allocator<T>,
              size_t N    = 4>
    using parallel_flat_hash_set_spinlock = 
        phmap::parallel_flat_hash_set<T, Hash, Eq, Alloc, N, SpinlockMutexAdapter>;

    template <class K, class V,
              class Hash  = PklEHashAdapter<K>,
              class Eq    = phmap::priv::hash_default_eq<K>,
              class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const K, V>>,
              size_t N    = 4>
    using parallel_node_hash_map_spinlock = 
        phmap::parallel_node_hash_map<K, V, Hash, Eq, Alloc, N, SpinlockMutexAdapter>;

    template <class T,
              class Hash  = PklEHashAdapter<T>,
              class Eq    = phmap::priv::hash_default_eq<T>,
              class Alloc = phmap::priv::Allocator<T>,
              size_t N    = 4>
    using parallel_node_hash_set_spinlock = 
        phmap::parallel_node_hash_set<T, Hash, Eq, Alloc, N, SpinlockMutexAdapter>;

    // Write-priority lock variants
    template <class K, class V,
              class Hash  = PklEHashAdapter<K>,
              class Eq    = phmap::priv::hash_default_eq<K>,
              class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const K, V>>,
              size_t N    = 4>
    using parallel_flat_hash_map_write_priority = 
        phmap::parallel_flat_hash_map<K, V, Hash, Eq, Alloc, N, SpinlockWritePriorityMutexAdapter>;

    template <class T,
              class Hash  = PklEHashAdapter<T>,
              class Eq    = phmap::priv::hash_default_eq<T>,
              class Alloc = phmap::priv::Allocator<T>,
              size_t N    = 4>
    using parallel_flat_hash_set_write_priority = 
        phmap::parallel_flat_hash_set<T, Hash, Eq, Alloc, N, SpinlockWritePriorityMutexAdapter>;

    template <class K, class V,
              class Hash  = PklEHashAdapter<K>,
              class Eq    = phmap::priv::hash_default_eq<K>,
              class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const K, V>>,
              size_t N    = 4>
    using parallel_node_hash_map_write_priority = 
        phmap::parallel_node_hash_map<K, V, Hash, Eq, Alloc, N, SpinlockWritePriorityMutexAdapter>;

    template <class T,
              class Hash  = PklEHashAdapter<T>,
              class Eq    = phmap::priv::hash_default_eq<T>,
              class Alloc = phmap::priv::Allocator<T>,
              size_t N    = 4>
    using parallel_node_hash_set_write_priority = 
        phmap::parallel_node_hash_set<T, Hash, Eq, Alloc, N, SpinlockWritePriorityMutexAdapter>;


} // namespace ThreadSafeContainers
} // namespace PklE
