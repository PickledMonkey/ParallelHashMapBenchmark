#pragma once

#include <stdint.h>
#include "atomic_util.h"
#include "memory_util.h"
#include "sized_byte_type.h"
#include "logging_util.h"

namespace PklE
{
namespace CoreTypes
{
    template<typename T, uint32_t Size_T, uint32_t Alignment_T = alignof(std::max_align_t)>
    class FixedSizeObjectPool
    {
        // Verify that Size_T is greater than zero and is a power of two
        static_assert(Size_T > 0, "FixedSizeObjectPool: Size_T must be greater than zero.");
        static_assert((Size_T & (Size_T - 1)) == 0, "FixedSizeObjectPool: Size_T must be a power of two.");


        inline static constexpr uint32_t c_dataSize = sizeof(T);

        using ThisPoolType = FixedSizeObjectPool<T, Size_T, Alignment_T>;

        struct Node
        {
            alignas(Alignment_T) u_char data[c_dataSize];
        };
        //Adding 1 because we need the extra index to represent the "invalid" index in the free list
        using IndexType = typename Util::SizeToIndexType<Size_T + 1>::IndexType;
        inline static constexpr IndexType c_InvalidIndex = Size_T;
        inline static constexpr IndexType c_maxNumNodes = static_cast<IndexType>(Size_T);
        using ByteType = typename Util::SizeToByteType<Size_T>::ByteType;
        inline static constexpr uint8_t c_bitIndexSize = Util::SizeToByteType<Size_T>::c_bitIndexSize;
        
        // Compile-time log2 calculation for bit shift optimization
        inline static constexpr uint8_t Log2BitIndexSize()
        {
            uint8_t log2 = 0;
            uint8_t val = c_bitIndexSize;
            while (val > 1) { val >>= 1; ++log2; }
            return log2;
        }
        inline static constexpr uint8_t c_bitIndexSizeLog2 = Log2BitIndexSize();
        
        // Fast division and modulus for power-of-two c_bitIndexSize
        template<typename IntType>
        inline static constexpr IntType FastDivBitIndexSize(IntType value)
        {
            return value >> c_bitIndexSizeLog2;
        }
        
        template<typename IntType>
        inline static constexpr IntType FastModBitIndexSize(IntType value)
        {
            return value & (static_cast<IntType>(c_bitIndexSize) - 1);
        }
        
        inline static constexpr bool c_sizeIsMultipleOfBitIndex = (c_maxNumNodes % static_cast<IndexType>(c_bitIndexSize)) == 0;
        static_assert(c_sizeIsMultipleOfBitIndex == true, "Because bitIndexSize is a power of two, and Size_T is a power of two, c_maxNumNodes must be a multiple of c_bitIndexSize.");

        inline static constexpr IndexType c_numBytesAllocatedBits = FastDivBitIndexSize(c_maxNumNodes) + (c_sizeIsMultipleOfBitIndex ? 0 : 1);
        static_assert((c_numBytesAllocatedBits & (c_numBytesAllocatedBits - 1)) == 0, "FixedSizeObjectPool: c_numBytesAllocatedBits must be a power of two.");

        alignas(Alignment_T) Node nodes[c_maxNumNodes];
        PKLE_DECLARE_ATOMIC_ALIGNED(IndexType, numAllocated) = 0;
        PKLE_DECLARE_ATOMIC_ALIGNED(ByteType, bAllocatedBits[c_numBytesAllocatedBits]) = {0};
        IndexType cachedIteratorIndex = 0;

        public:
        struct Iterator
        {
            ThisPoolType* pPool = nullptr;
            IndexType byteIndex = c_numBytesAllocatedBits;
            uint8_t bitIndex = 0;

            Iterator()
            {

            }

            Iterator(FixedSizeObjectPool* pPool, IndexType index) : pPool(pPool)
            {
                if(pPool->numAllocated == 0)
                {
                    byteIndex = c_numBytesAllocatedBits;
                    bitIndex = 0;
                }
                else if(index >= c_maxNumNodes)
                {
                    byteIndex = c_numBytesAllocatedBits;
                    bitIndex = 0;
                }
                else
                {
                    byteIndex = FastDivBitIndexSize(index);
                    bitIndex = static_cast<uint8_t>(FastModBitIndexSize(index));

                    ByteType mask = 1;
                    bool bIsAllocated = (pPool->bAllocatedBits[byteIndex] & (mask)) > 0;
                    if(!bIsAllocated)
                    {
                        operator++();
                    }
                }
            }

            T* operator*()
            {
                IndexType index = (byteIndex << c_bitIndexSizeLog2) + bitIndex;
                T* pData = pPool->LookupByIndex(index);
                return pData;
            }

            const T* operator*() const
            {
                IndexType index = (byteIndex << c_bitIndexSizeLog2) + bitIndex;
                const T* pData = pPool->LookupByIndex(index);
                return pData;
            }

            Iterator& operator++()
            {
                bool bIsAllocated = false;
                ByteType mask = static_cast<ByteType>(1) << static_cast<ByteType>(bitIndex);
                do
                {
                    bitIndex++;
                    if(bitIndex == c_bitIndexSize)
                    {
                        bitIndex = 0;
                        mask = 1;
                        byteIndex++;

                        if(byteIndex >= c_numBytesAllocatedBits)
                        {
                            break;
                        }
                    }
                    else
                    {
                        mask <<= 1;
                    }
                    bIsAllocated = (pPool->bAllocatedBits[byteIndex] & (mask)) > 0;
                } while (!bIsAllocated && (byteIndex < c_numBytesAllocatedBits));
                return *this;
            }

            bool operator!=(const Iterator& other) const
            {
                return bitIndex != other.bitIndex || byteIndex != other.byteIndex;
            }

            bool operator==(const Iterator& other) const
            {
                return bitIndex == other.bitIndex && byteIndex == other.byteIndex;
            }
        };

        Iterator begin()
        {
            Iterator it(this, 0);
            return it;
        }

        Iterator end()
        {
            Iterator it(this, c_maxNumNodes);
            return it;
        }

        FixedSizeObjectPool()
        {
        }

        ~FixedSizeObjectPool()
        {
            for(IndexType byteIndex = 0; byteIndex < c_numBytesAllocatedBits; ++byteIndex)
            {
                const ByteType mask = 1;
                ByteType byte = bAllocatedBits[byteIndex];
                for(uint8_t bitIndex = 0; byte != 0; ++bitIndex)
                {
                    if(byte & mask)
                    {
                        const IndexType nodeIndex = (byteIndex << c_bitIndexSizeLog2) + bitIndex;
                        Node& node = nodes[nodeIndex];
                        T* pData = reinterpret_cast<T*>(node.data);
                        Util::Destroy(pData);
                        --numAllocated;
                    }
                    byte >>= 1;
                }
                bAllocatedBits[byteIndex] = 0;

                if(!numAllocated)
                {
                    break;
                }
            }
        }

        inline bool PtrIsElementInStaticArray(const T* pData)
        {
            const bool bIsElementInStaticArray = 
                (pData >= reinterpret_cast<T*>(nodes)) && 
                (pData < reinterpret_cast<T*>(nodes + c_maxNumNodes)) &&
                ((reinterpret_cast<uintptr_t>(pData) - reinterpret_cast<uintptr_t>(nodes)) % c_dataSize == 0);
            return bIsElementInStaticArray;
        } 
        
        inline IndexType GetIndex(const T* pData) const
        {
            const Node* pNode = reinterpret_cast<const Node*>(pData);
            if(pNode >= nodes)
            {
                const uintptr_t ptrDiff = reinterpret_cast<uintptr_t>(pNode) - reinterpret_cast<uintptr_t>(nodes);
                const uintptr_t nodeIndex = (ptrDiff) / sizeof(Node);
                const uintptr_t remainder = (ptrDiff) % sizeof(Node);
                if((nodeIndex < c_maxNumNodes) && (remainder == 0))
                {
                    return static_cast<IndexType>(nodeIndex);
                }
            }
            return c_InvalidIndex;
        }

        inline bool IsFull() const
        {
            const bool bIsFull = (numAllocated == c_maxNumNodes);
            return bIsFull;
        }

        inline bool HasFreeSpace() const
        {
            const bool bHasFreeSpace = !IsFull();
            return bHasFreeSpace;
        }

        inline bool IsAllocated(const IndexType nodeIndex) const
        {
            if(nodeIndex != c_InvalidIndex)
            {
                //Find the bit corresponding to the node index in the bAllocatedBits array
                const IndexType byteIndex = FastDivBitIndexSize(nodeIndex);
                const IndexType bitIndex = FastModBitIndexSize(nodeIndex);
                const ByteType byte = bAllocatedBits[byteIndex];
                const ByteType mask = static_cast<ByteType>(1) << static_cast<ByteType>(bitIndex);
                const bool bAllocated = (byte & mask) != 0;
                return bAllocated;
            }
            return false;
        }

        inline bool SetAllocated(const IndexType nodeIndex)
        {
            bool bSuccess = false;
            if(nodeIndex < c_maxNumNodes)
            {
                //Find the bit corresponding to the node index in the bAllocatedBits array
                const IndexType byteIndex = FastDivBitIndexSize(nodeIndex);
                const IndexType bitIndex = FastModBitIndexSize(nodeIndex);
                const ByteType mask = static_cast<ByteType>(1) << static_cast<ByteType>(bitIndex);

                //Atomically set the bit
                ByteType prevValue = Util::AtomicOr<ByteType>(bAllocatedBits[byteIndex], mask);
                bSuccess = ((prevValue & mask) == 0);
            }
            return bSuccess;
        }


        inline bool SetDeallocated(const IndexType nodeIndex)
        {
            bool bSuccess = false;
            if(nodeIndex < c_maxNumNodes)
            {
                //Find the bit corresponding to the node index in the bAllocatedBits array
                const IndexType byteIndex = FastDivBitIndexSize(nodeIndex);
                const IndexType bitIndex = FastModBitIndexSize(nodeIndex);
                const ByteType mask = ~(static_cast<ByteType>(1) << static_cast<ByteType>(bitIndex));
                
                //Atomically clear the bit
                ByteType prevVal = Util::AtomicAnd<ByteType>(bAllocatedBits[byteIndex], mask);
                bSuccess = ((prevVal & ~mask) != 0);
            }
            return bSuccess;
        }

        T* LookupByIndex(IndexType index)
        {
            T* pData = nullptr;
            if(index != c_InvalidIndex)
            {
                if(index < c_maxNumNodes)
                {
                    Node& node = nodes[index];
                    if(IsAllocated(index))
                    {
                        pData = reinterpret_cast<T*>(node.data);
                    }
                }
            }
            return pData;
        }

        const T* LookupByIndex(IndexType index) const
        {
            const T* pData = nullptr;
            if(index != c_InvalidIndex)
            {
                if(index < c_maxNumNodes)
                {
                    const Node& node = nodes[index];
                    if(IsAllocated(index))
                    {
                        pData = reinterpret_cast<const T*>(node.data);
                    }
                }
            }
            return pData;
        }

        template<typename... Args_T>
        T* Reserve(Args_T&&... args)
        {
            T* pSelectedNode = nullptr;            
            IndexType currentIndex = cachedIteratorIndex;
            for(IndexType i = 0; i < c_maxNumNodes; ++i)
            {
                currentIndex = (currentIndex >= c_maxNumNodes) ? 0 : currentIndex;
                if(numAllocated >= c_maxNumNodes)
                {
                    break;
                }

                if(!IsAllocated(currentIndex))
                {
                    bool bAllocated = SetAllocated(currentIndex);
                    if(bAllocated)
                    {
                        cachedIteratorIndex = currentIndex + 1;
                        pSelectedNode = Util::Construct<T>(nodes[currentIndex].data, std::forward<Args_T>(args)...);
                        IndexType nextNumAllocated = Util::AtomicIncrement<IndexType>(numAllocated);
                        break;
                    }
                }
                ++currentIndex;
            }
            return pSelectedNode;
        }

        void* ReserveRaw()
        {
            void* pSelectedNode = nullptr;            
            IndexType currentIndex = cachedIteratorIndex;
            for(IndexType i = 0; i < c_maxNumNodes; ++i)
            {
                currentIndex = (currentIndex >= c_maxNumNodes) ? 0 : currentIndex;
                if(numAllocated >= c_maxNumNodes)
                {
                    break;
                }

                if(!IsAllocated(currentIndex))
                {
                    bool bAllocated = SetAllocated(currentIndex);
                    if(bAllocated)
                    {
                        cachedIteratorIndex = currentIndex + 1;
                        pSelectedNode = reinterpret_cast<void*>(nodes[currentIndex].data);
                        IndexType nextNumAllocated = Util::AtomicIncrement<IndexType>(numAllocated);
                        break;
                    }
                }
                ++currentIndex;
            }
            return pSelectedNode;
        }

        bool Release(const T* pData)
        {
            bool bReleased = false;

            //Verify that the pointer is within the nodes array
            IndexType nodeIndex = GetIndex(pData);
            if(nodeIndex < c_maxNumNodes)
            {
                const Node* pNode = reinterpret_cast<const Node*>(pData);
                if(IsAllocated(nodeIndex))
                {
                    Util::Destroy(reinterpret_cast<T*>(nodes[nodeIndex].data));
                    bool bDeallocated = SetDeallocated(nodeIndex);
                    if(bDeallocated)
                    {
                        IndexType nextNumAllocated = Util::AtomicDecrement<IndexType>(numAllocated);
                        bReleased = true;
                    }
                    else
                    {
                        PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "FixedSizeObjectPool::Release: Double free detected or free of unallocated object.");
                    }
                }
            }            
            return bReleased;
        }

        bool ReleaseRaw(const void* pData)
        {
            bool bReleased = false;

            //Verify that the pointer is within the nodes array
            IndexType nodeIndex = GetIndex(reinterpret_cast<const T*>(pData));
            if(nodeIndex < c_maxNumNodes)
            {
                const Node* pNode = reinterpret_cast<const Node*>(pData);
                if(IsAllocated(nodeIndex))
                {
                    bool bDeallocated = SetDeallocated(nodeIndex);
                    if(bDeallocated)
                    {
                        IndexType nextNumAllocated = Util::AtomicDecrement<IndexType>(numAllocated);
                        bReleased = true;
                    }
                    else
                    {
                        PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "FixedSizeObjectPool::ReleaseRaw: Double free detected or free of unallocated object.");
                    }
                }
            }            
            return bReleased;
        }

        void Clear()
        {
            //Destroy all allocated objects
            for(IndexType i = 0; i < c_maxNumNodes; ++i)
            {
                if(IsAllocated(i))
                {
                    Node& node = nodes[i];
                    T* pData = reinterpret_cast<T*>(node.data);
                    Util::Destroy(pData);
                }
            }

            //Reset the allocated bits
            for(IndexType byteIndex = 0; byteIndex < c_numBytesAllocatedBits; ++byteIndex)
            {
                bAllocatedBits[byteIndex] = 0;
            }

            numAllocated = 0;
        }
    };
}; //end namespace CoreTypes
}; //end namespace PklE