#pragma once

#include "paging_object_pool.h"
#include "vector_array.h"
#include "spin_lock.h"
#include "hash_type.h"
#include "magic_num_util.h"

#include "simple_linked_list.h"

namespace PklE
{
namespace ThreadsafeContainers
{
    template<typename Key_T, typename Value_T, uint32_t PageSize_T = 8, uint32_t NumInnerMaps_T = 4>
    class HashMap
    {
        // static assert that NumInnerMaps_T is a power of two
        static_assert((NumInnerMaps_T & (NumInnerMaps_T - 1)) == 0, "HashMap: NumInnerMaps_T must be a power of two.");

        public:
        using ThisHashMapType = HashMap<Key_T, Value_T, PageSize_T, NumInnerMaps_T>;
        struct KeyValuePair
        {
            const Key_T key;
            Value_T value;

            template<typename... Args>
            KeyValuePair(const Key_T& key, Args&&... args) : key(key), value(std::forward<Args>(args)...)
            {
            }

            void ForceChangeKey(const Key_T& newKey)
            {
                //Unsafe cast to change the key. Use with caution.
                Key_T* pKey = const_cast<Key_T*>(&key);
                *pKey = newKey;
            }
        };

        private:

        struct Node : KeyValuePair
        {
            inline static constexpr uint32_t c_invalidBucket = 0xFFFFFFFF;
            inline static constexpr uint32_t c_reassigningBucket = 0xFFFFFFFE;

            Node* pNext = nullptr;
            uint32_t bucket = c_invalidBucket;

            template<typename... Args>
            Node(const Key_T& key, Args&&... args) : KeyValuePair(key, std::forward<Args>(args)...)
            {
            }
        };

        template<typename Comparable_T>
        struct KeyComparator
        {
            static int Compare(const Key_T& a, const Comparable_T& b)
            {
                if(a < b)
                {
                    return -1;
                }
                else if(a > b)
                {
                    return 1;
                }
                return 0;
            }
        };

        struct Bucket
        {
            mutable SimpleLinkedList<Node> list;

            Bucket() = default;

            bool Insert_Lockless(Node* pNewNode)
            {
                return list.Insert_Unsafe(pNewNode);
            }

            bool Insert_Concurrent(Node* pNewNode)
            {
                return list.Insert(pNewNode);
            }

            bool InsertUnique_Lockless(Node* pNewNode)
            {
                const Node* pExistingNode = Find_Lockless(pNewNode->key);
                if(!pExistingNode)
                {
                    return list.Insert_Unsafe(pNewNode);
                }
                return false;
            }

            bool InsertUnique_Concurrent(Node* pNewNode)
            {
                bool bInserted = list.Insert(pNewNode);
                if(bInserted)
                {
                    const Node* pLastExistingNode = list.template FindLast<Key_T, KeyComparator<Key_T>>(pNewNode->key);
                    if(pLastExistingNode != pNewNode)
                    {
                        //A node with the same key already existed, remove the newly inserted one
                        Node* pRemovedNode = list.EraseNode(pNewNode);
                        PKLE_ASSERT_SYSTEM_ERROR_MSG(pRemovedNode == pNewNode, "HashMap::Bucket::InsertUnique_Concurrent: Failed to remove duplicate node after detecting existing key.");
                        bInserted = false;
                    }
                }
                return bInserted;
            }

            template<typename Comparable_T>
            const Node* Find_Lockless(const Comparable_T& key) const
            {
                return list.template Find_Unsafe<Comparable_T, KeyComparator<Comparable_T>>(key);
            }

            template<typename Comparable_T>
            const Node* Find_Concurrent(const Comparable_T& key) const
            {
                return list.template Find<Comparable_T, KeyComparator<Comparable_T>>(key);
            }

            template<typename Comparable_T>
            Node* Find_Lockless(const Comparable_T& key)
            {
                return list.template Find_Unsafe<Comparable_T, KeyComparator<Comparable_T>>(key);
            }

            template<typename Comparable_T>
            Node* Find_Concurrent(const Comparable_T& key)
            {
                return list.template Find<Comparable_T, KeyComparator<Comparable_T>>(key);
            }

            template<typename Comparable_T>
            Node* Erase_Lockless(const Comparable_T& key)
            {
                return list.template Erase_Unsafe<Comparable_T, KeyComparator<Comparable_T>>(key);
            }

            template<typename Comparable_T>
            Node* Erase_Concurrent(const Comparable_T& key)
            {
                return list.template Erase<Comparable_T, KeyComparator<Comparable_T>>(key);
            }

            // Resets the bucket list to an empty state without cleaning up nodes
            void Reset_Lockless()
            {
                list.Reset_Unsafe();
            }
        };

        inline static constexpr uint64_t c_numInnerMaps = NumInnerMaps_T;
        inline static constexpr uint64_t c_innerMapIndexMask = c_numInnerMaps - 1;
        using PoolType = CoreTypes::PagingObjectPool<Node, PageSize_T>;
        PoolType sharedPool;

        struct InnerMap
        {
            PoolType& pool;
            Bucket* buckets = nullptr;
            uint32_t count = 0;
            uint32_t fillCapacity = 0;
            uint32_t numBuckets = 0;
            mutable CoreTypes::CountingSpinlock lock;

            InnerMap(PoolType& sharedPool) : pool(sharedPool)
            {

            }

            ~InnerMap()
            {
                if(buckets)
                {
                    for(uint32_t i = 0; i < numBuckets; ++i)
                    {
                        //Destroy old buckets
                        buckets[i].Reset_Lockless();
                    }
                    Util::Free(buckets);
                    buckets = nullptr;
                    numBuckets = 0;
                }
            }

            uint32_t GetIndex(const uint64_t hash) const
            {
                //Table is always power of two sized, so we can use bitmasking
                return static_cast<uint32_t>(hash & (static_cast<uint64_t>(numBuckets) - 1));
            }

            void Resize(const uint32_t newNumBuckets)
            {   
                Bucket* pNewBuckets = Util::NewArray<Bucket>(newNumBuckets);

                //Iterate through the buckets and move nodes to the new buckets
                const uint32_t oldNumBuckets = numBuckets;
                numBuckets = newNumBuckets;
                for(uint32_t i = 0; i < oldNumBuckets; ++i)
                {
                    Bucket& oldBucket = buckets[i];
                    Node* pNode = oldBucket.list.GetHead();
                    while(pNode)
                    {
                        Node* pNextNode = pNode->pNext;
                        pNode->pNext = nullptr;

                        const uint64_t hash = Util::HashType::Hash64(pNode->key);
                        const uint32_t newBucketIndex = GetIndex(hash);

                        pNode->bucket = newBucketIndex;
                        bool bInserted = pNewBuckets[newBucketIndex].Insert_Lockless(pNode);
                        if(!bInserted)
                        {
                            PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "HashMap::Resize: Insertion into new bucket failed during resize. This should never happen.");
                        }

                        pNode = pNextNode;
                    }
                }

                if(buckets)
                {
                    for(uint32_t i = 0; i < oldNumBuckets; ++i)
                    {
                        //Destroy old buckets
                        buckets[i].Reset_Lockless();
                    }
                    Util::Free(buckets);
                }

                buckets = pNewBuckets;
                fillCapacity = static_cast<uint32_t>((numBuckets * 7) / 8); //Having a fill capacity of 87.5% seems to be a good balance between memory usage and performance.
            }

            template<typename... Args>
            KeyValuePair* Insert_Lockless(const uint64_t hash, const Key_T& key, Args&&... args)
            {
                ++count;
                if(count > fillCapacity)
                {
                    const uint32_t newNumBuckets = PklE::Util::GetNextPowerOfTwo(count * 2);
                    Resize(newNumBuckets);
                }

                const uint32_t bucket = GetIndex(hash);

                Node* pNewNode = nullptr;
                bool bHasExisting = buckets[bucket].Find_Lockless(key) != nullptr;
                if(!bHasExisting)
                {
                    pNewNode = pool.Reserve(key, std::forward<Args>(args)...);
                    pNewNode->bucket = bucket;
                    bool bInserted = buckets[bucket].Insert_Lockless(pNewNode);
                    if(!bInserted)
                    {
                        //Insertion failed for some reason, free the node and return nullptr
                        pool.Release(pNewNode);
                        --count;
                        pNewNode = nullptr;
                    }
                }

                return static_cast<KeyValuePair*>(pNewNode);
            }

            template<typename... Args>
            KeyValuePair* Insert_Concurrent(const uint64_t hash, const Key_T& key, Args&&... args)
            {
                CoreTypes::ScopedWriteSpinLock writeLock(lock);
                return Insert_Lockless(hash, key, std::forward<Args>(args)...);
            }

            template<typename Comparable_T>
            inline KeyValuePair* Find_Lockless(const uint64_t hash,const Comparable_T& key)
            {
                const uint32_t bucket = GetIndex(hash);
                Node* pFoundNode = buckets[bucket].Find_Lockless(key);
                return static_cast<KeyValuePair*>(pFoundNode);
            }

            template<typename Comparable_T>
            inline KeyValuePair* Find_Concurrent(const uint64_t hash, const Comparable_T& key)
            {
                CoreTypes::ScopedReadSpinLock readLock(lock);
                return Find_Lockless(hash, key);
            }

            template<typename Comparable_T>
            inline const KeyValuePair* Find_Lockless(const uint64_t hash, const Comparable_T& key) const
            {
                const uint32_t bucket = GetIndex(hash);
                const Node* pFoundNode = buckets[bucket].Find_Lockless(key);
                return static_cast<const KeyValuePair*>(pFoundNode);
            }

            template<typename Comparable_T>
            inline const KeyValuePair* Find_Concurrent(const uint64_t hash,const Comparable_T& key) const
            {
                CoreTypes::ScopedReadSpinLock readLock(lock);
                return Find_Lockless(hash, key);
            }

            template<typename Comparable_T>
            bool Remove_Lockless(const uint64_t hash, const Comparable_T& key)
            {
                bool bRemoved = false;
                const uint32_t bucket = GetIndex(hash);

                Node* pNode = buckets[bucket].Erase_Lockless(key);
                if(pNode)
                {
                    if(pNode->bucket != Node::c_reassigningBucket)
                    {
                        pool.Release(pNode);
                    }
                    --count;
                    bRemoved = true;
                }
                return bRemoved;
            }

            template<typename Comparable_T>
            bool Remove_Concurrent(const uint64_t hash,const Comparable_T& key)
            {
                CoreTypes::ScopedWriteSpinLock writeLock(lock);
                return Remove_Lockless(hash, key);
            }

            template<>
            bool Remove_Lockless<KeyValuePair>(const uint64_t hash, const KeyValuePair& value)
            {
                bool bRemoved = false;
                const Node* pNode = reinterpret_cast<const Node*>(&value);
                const uint32_t bucket = pNode->bucket;
                if(bucket < numBuckets)
                {
                    Node* pRemovedNode = buckets[bucket].Erase_Lockless(pNode->key);
                    if(pRemovedNode)
                    {
                        if(pRemovedNode->bucket != Node::c_reassigningBucket)
                        {
                            pool.Release(pRemovedNode);
                        }
                        --count;
                        bRemoved = true;
                    }
                }
                return bRemoved;
            }

            template<>
            bool Remove_Concurrent<KeyValuePair>(const uint64_t hash, const KeyValuePair& value)
            {
                CoreTypes::ScopedWriteSpinLock writeLock(lock);
                return Remove_Lockless(hash, value);
            }

            bool ReKey_Lockless(const uint64_t hash, const uint64_t newHash, const KeyValuePair& value, const Key_T& newKey)
            {
                bool bRekeyed = false;
                Node* pNode = reinterpret_cast<Node*>(const_cast<KeyValuePair*>(&value));
                const uint32_t oldBucket = pNode->bucket;

                const uint32_t newBucket = GetIndex(newHash);

                if((oldBucket < numBuckets))
                {
                    if(oldBucket != newBucket)
                    {
                        pNode->bucket = Node::c_reassigningBucket; //Mark the node as being reassigned to prevent destruction during removal
                        Node* pRemovedNode = buckets[oldBucket].Erase_Lockless(pNode->key);
                        pNode->ForceChangeKey(newKey);
                        if(pRemovedNode)
                        {
                            pNode->bucket = newBucket;

                            bRekeyed = buckets[newBucket].Insert_Lockless(pNode);
                            if(!bRekeyed)
                            {
                                PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "ReKey_Lockless: Insertion into new bucket failed during rekeying. The node has been lost. This should never happen.");
                            }
                        }
                    }
                    else
                    {
                        //Same bucket, just change the key
                        pNode->ForceChangeKey(newKey);
                        bRekeyed = true;
                    }
                }
                return bRekeyed;
            }

            bool ReKey_Concurrent(const uint64_t hash, const uint64_t newHash, const KeyValuePair& value, const Key_T& newKey)
            {
                bool bRekeyed = false;
                Node* pNode = reinterpret_cast<Node*>(const_cast<KeyValuePair*>(&value));
                {
                    CoreTypes::ScopedReadSpinLock readLock(lock);

                    const uint32_t newBucket = GetIndex(newHash);

                    bool bAttemptedReKey = false;
                    do
                    {
                        const uint32_t oldBucket = pNode->bucket;
                        if(oldBucket < numBuckets)
                        {
                            bAttemptedReKey = Util::AtomicCompareExchangeU32(pNode->bucket, Node::c_reassigningBucket, oldBucket);
                            if(bAttemptedReKey)
                            {
                                if(oldBucket != newBucket)
                                {
                                    CoreTypes::ScopedWriteSpinLock writeLock(std::move(readLock));
                                    Node* pRemovedNode = buckets[oldBucket].Erase_Lockless(pNode->key);
                                    if(pRemovedNode)
                                    {
                                        pNode->ForceChangeKey(newKey);
                                        pNode->bucket = newBucket;
                                        bRekeyed = buckets[newBucket].Insert_Lockless(pNode);
                                        if(!bRekeyed)
                                        {
                                            PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "ReKey_Concurrent: Insertion into new bucket failed during rekeying. The node has been lost. This should never happen.");
                                        }
                                    }
                                }
                                else
                                {
                                    //Same bucket, just change the key
                                    pNode->ForceChangeKey(newKey);
                                    pNode->bucket = newBucket;
                                    bRekeyed = true;
                                }
                            }
                        }
                    } while(!bAttemptedReKey);
                }
                return bRekeyed;
            }

            bool ReKey_Lockless(const Key_T& key, const Key_T& newKey)
            {
                KeyValuePair* pValue = Find_Lockless(key);
                if(pValue)
                {
                    return ReKey_Lockless(*pValue, newKey);
                }
                return false;
            }

            bool ReKey_Concurrent(const Key_T& key, const Key_T& newKey)
            {
                KeyValuePair* pValue = Find_Concurrent(key);
                if(pValue)
                {
                    return ReKey_Concurrent(*pValue, newKey);
                }
                return false;
            }

            void Clear_Lockless()
            {
                count = 0;
                for(uint32_t i = 0; i < numBuckets; ++i)
                {
                    buckets[i].Reset_Lockless();
                }
            }
        };

        uint32_t totalCount = 0;
        InnerMap innerMaps[c_numInnerMaps];

    public:

        struct Iterator
        {
            using NodeIterator = typename PoolType::Iterator;
            NodeIterator nodeIterator;
            PoolType& thisPool;

            Iterator(PoolType& thisPool, bool bIsBegin) : thisPool(thisPool)
            {
                if(bIsBegin)
                {
                    nodeIterator = thisPool.begin();
                }
                else
                {
                    nodeIterator = thisPool.end();
                }
            }

            Iterator& operator++()
            {
                ++nodeIterator;
                return *this;
            }

            KeyValuePair& operator*()
            {
                Node* pNode = *nodeIterator;
                return static_cast<KeyValuePair&>(*pNode);
            }

            const KeyValuePair& operator*() const
            {
                const Node* pNode = *nodeIterator;
                return static_cast<const KeyValuePair&>(*pNode);
            }

            bool operator!=(const Iterator& other) const
            {
                return nodeIterator != other.nodeIterator;
            }

            bool operator==(const Iterator& other) const
            {
                return nodeIterator == other.nodeIterator;
            }
        };

        Iterator begin()
        {
            return Iterator(sharedPool, true);
        }

        Iterator end()
        {
            return Iterator(sharedPool, false);
        }

        inline uint32_t GetInnerMapIndex(const uint64_t hash) const
        {
            // This function assumes c_numInnerMaps is a power of two
            return hash & (c_innerMapIndexMask);
        }

    private:
        template<std::size_t... Is>
        HashMap(std::index_sequence<Is...>) : sharedPool(), innerMaps { (static_cast<void>(Is), InnerMap(sharedPool))... }
        {
        }

    public:
        HashMap() : HashMap(std::make_index_sequence<c_numInnerMaps>{})
        {
        }

        ~HashMap()
        {
            //Inner maps will clean themselves up in their destructors
        }

        template<typename... Args>
        KeyValuePair* Insert_Lockless(const Key_T& key, Args&&... args)
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            KeyValuePair* pAdded = innerMaps[mapIndex].Insert_Lockless(hash, key, std::forward<Args>(args)...);
            if(pAdded)
            {
                ++totalCount;
            }
            return pAdded;
        }

        template<typename... Args>
        KeyValuePair* Insert_Concurrent(const Key_T& key, Args&&... args)
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            KeyValuePair* pAdded = innerMaps[mapIndex].Insert_Concurrent(hash, key, std::forward<Args>(args)...);
            if(pAdded)
            {
                Util::AtomicIncrementU32(totalCount);
            }
            return pAdded;
        }

        template<typename Comparable_T>
        KeyValuePair* Find_Lockless(const Comparable_T& key)
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            return innerMaps[mapIndex].Find_Lockless(hash, key);
        }

        template<typename Comparable_T>
        KeyValuePair* Find_Concurrent(const Comparable_T& key)
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            return innerMaps[mapIndex].Find_Concurrent(hash, key);
        }

        template<typename Comparable_T>
        const KeyValuePair* Find_Lockless(const Comparable_T& key) const
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            return innerMaps[mapIndex].Find_Lockless(hash, key);
        }

        template<typename Comparable_T>
        const KeyValuePair* Find_Concurrent(const Comparable_T& key) const
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            return innerMaps[mapIndex].Find_Concurrent(hash, key);
        }

        template<typename Comparable_T>
        bool Remove_Lockless(const Comparable_T& key)
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            bool bRemoved = innerMaps[mapIndex].Remove_Lockless(hash, key);
            if(bRemoved)
            {
                --totalCount;
            }
            return bRemoved;
        }

        template<typename Comparable_T>
        bool Remove_Concurrent(const Comparable_T& key)
        {
            const uint64_t hash = Util::HashType::Hash64(key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            bool bRemoved = innerMaps[mapIndex].Remove_Concurrent(hash, key);
            if(bRemoved)
            {
                Util::AtomicDecrementU32(totalCount);
            }
            return bRemoved;
        }

        template<>
        bool Remove_Lockless<KeyValuePair>(const KeyValuePair& value)
        {
            const Node* pNode = reinterpret_cast<const Node*>(&value);
            const uint64_t hash = Util::HashType::Hash64(pNode->key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            bool bRemoved = innerMaps[mapIndex].Remove_Lockless(hash, value);
            if(bRemoved)
            {
                --totalCount;
            }
            return bRemoved;
        }

        template<>
        bool Remove_Concurrent<KeyValuePair>(const KeyValuePair& value)
        {
            const Node* pNode = reinterpret_cast<const Node*>(&value);
            const uint64_t hash = Util::HashType::Hash64(pNode->key);
            const uint32_t mapIndex = GetInnerMapIndex(hash);
            bool bRemoved = innerMaps[mapIndex].Remove_Concurrent(hash, value);
            if(bRemoved)
            {
                Util::AtomicDecrementU32(totalCount);
            }
            return bRemoved;
        }

        bool ReKey_Lockless(const KeyValuePair& value, const Key_T& newKey)
        {
            bool bRekeyed = false;
            Node* pNode = reinterpret_cast<Node*>(const_cast<KeyValuePair*>(&value));
            const uint64_t oldHash = Util::HashType::Hash64(pNode->key);
            const uint32_t oldMapIndex = GetInnerMapIndex(oldHash);

            const uint64_t newHash = Util::HashType::Hash64(newKey);
            const uint32_t newMapIndex = GetInnerMapIndex(newHash);

            if(oldMapIndex == newMapIndex)
            {
                bRekeyed = innerMaps[oldMapIndex].ReKey_Lockless(oldHash, newHash, value, newKey);
            }
            else
            {
                //Different inner maps, need to remove and re-insert
                pNode->bucket = Node::c_reassigningBucket; //Mark the node as being reassigned to prevent destruction during removal
                bool bRemovedNode = innerMaps[oldMapIndex].Remove_Lockless(oldHash, value);
                if(bRemovedNode)
                {
                    pNode->ForceChangeKey(newKey);
                    pNode->bucket = Node::c_invalidBucket; //Reset bucket before re-insert
                    bRekeyed = innerMaps[newMapIndex].Insert_Lockless(newHash, pNode->key, std::move(pNode->value));
                    if(!bRekeyed)
                    {
                        PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "ReKey_Lockless: Insertion into new inner map failed during rekeying. The node has been lost. This should never happen.");
                        --totalCount; //Adjust total count since the node is lost
                    }
                }
            }
            return bRekeyed;
        }

        bool ReKey_Concurrent(const KeyValuePair& value, const Key_T& newKey)
        {
            bool bRekeyed = false;
            Node* pNode = reinterpret_cast<Node*>(const_cast<KeyValuePair*>(&value));
            const uint64_t oldHash = Util::HashType::Hash64(pNode->key);
            const uint32_t oldMapIndex = GetInnerMapIndex(oldHash);

            const uint64_t newHash = Util::HashType::Hash64(newKey);
            const uint32_t newMapIndex = GetInnerMapIndex(newHash);

            if(oldMapIndex == newMapIndex)
            {
                bRekeyed = innerMaps[oldMapIndex].ReKey_Concurrent(oldHash, newHash, value, newKey);
            }
            else
            {
                //Different inner maps, need to remove and re-insert
                pNode->bucket = Node::c_reassigningBucket; //Mark the node as being reassigned to prevent destruction during removal
                bool bRemovedNode = innerMaps[oldMapIndex].Remove_Concurrent(oldHash, value);
                if(bRemovedNode)
                {
                    pNode->ForceChangeKey(newKey);
                    pNode->bucket = Node::c_invalidBucket; //Reset bucket before re-insert
                    bRekeyed = innerMaps[newMapIndex].Insert_Concurrent(newHash, pNode->key, std::move(pNode->value));
                    if(!bRekeyed)
                    {
                        PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "ReKey_Lockless: Insertion into new inner map failed during rekeying. The node has been lost. This should never happen.");
                        Util::AtomicDecrementU32(totalCount); //Adjust total count since the node is lost
                    }
                }
            }
            return bRekeyed;
        }

        bool ReKey_Lockless(const Key_T& key, const Key_T& newKey)
        {
            KeyValuePair* pValue = Find_Lockless(key);
            if(pValue)
            {
                return ReKey_Lockless(*pValue, newKey);
            }
            return false;
        }

        bool ReKey_Concurrent(const Key_T& key, const Key_T& newKey)
        {
            KeyValuePair* pValue = Find_Concurrent(key);
            if(pValue)
            {
                return ReKey_Concurrent(*pValue, newKey);
            }
            return false;
        }

        bool IsEmpty() const
        {
            return totalCount == 0;
        }

        uint32_t Size() const
        {
            return totalCount;
        }

        void Clear_Lockless()
        {
            totalCount = 0;
            
            for(uint32_t i = 0; i < c_numInnerMaps; ++i)
            {
                innerMaps[i].Clear_Lockless();
            }

            sharedPool.Clear();
        }

        void Reserve(uint32_t numElements)
        {
            uint32_t numElementsPlusFill = static_cast<uint32_t>((numElements * 8) / 7) + 1; //Account for fill capacity

            const uint32_t numElementsPerInnerMap = (numElementsPlusFill + c_numInnerMaps - 1) / c_numInnerMaps;
            for(uint32_t i = 0; i < c_numInnerMaps; ++i)
            {
                innerMaps[i].Resize(PklE::Util::GetNextPowerOfTwo(numElementsPerInnerMap));
            }
            sharedPool.PreallocateSpace(numElements);
        }

        // std::map-like interface wrappers
        bool insert(const std::pair<Key_T, Value_T>& pair)
        {
            return Insert_Concurrent(pair.first, pair.second) != nullptr;
        }

        bool insert_lockless(const std::pair<Key_T, Value_T>& pair)
        {
            return Insert_Lockless(pair.first, pair.second) != nullptr;
        }

        bool find(const Key_T& key, const Value_T*& pOutValue) const
        {
            const KeyValuePair* pPair = Find_Concurrent(key);
            if(pPair)
            {
                pOutValue = &(pPair->value);
                return true;
            }
            return false;
        }

        bool find_lockless(const Key_T& key, const Value_T*& pOutValue) const
        {
            const KeyValuePair* pPair = Find_Lockless(key);
            if(pPair)
            {
                pOutValue = &(pPair->value);
                return true;
            }
            return false;
        }

        bool find(const Key_T& key, Value_T*& pOutValue)
        {
            KeyValuePair* pPair = Find_Concurrent(key);
            if(pPair)
            {
                pOutValue = &(pPair->value);
                return true;
            }
            return false;
        }

        bool find_lockless(const Key_T& key, Value_T*& pOutValue)
        {
            KeyValuePair* pPair = Find_Lockless(key);
            if(pPair)
            {
                pOutValue = &(pPair->value);
                return true;
            }
            return false;
        }

        bool erase(const Key_T& key)
        {
            return Remove_Concurrent(key);
        }

        bool erase_lockless(const Key_T& key)
        {
            return Remove_Lockless(key);
        }

        bool rekey(const Key_T& key, const Key_T& newKey)
        {
            return ReKey_Concurrent(key, newKey);
        }

        bool rekey_lockless(const Key_T& key, const Key_T& newKey)
        {
            return ReKey_Lockless(key, newKey);
        }

        void clear()
        {
            Clear_Lockless();
        }

        size_t size() const
        {
            return Size();
        }

        void reserve(size_t numElements)
        {
            Reserve(static_cast<uint32_t>(numElements));
        }
    };



}; //end namespace ThreadsafeContainers
}; //end namespace PklE