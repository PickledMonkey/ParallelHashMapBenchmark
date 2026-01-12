#pragma once

#include "atomic_util.h"
#include "linked_list_util.h"
#include "logging_util.h"
#include "sized_byte_type.h"
#include "memory_util.h"
#include "fixedsize_object_pool.h"

#include <utility>

namespace PklE
{
namespace CoreTypes
{
    template<typename T, uint64_t PageSize_T = 0xF, uint64_t PageAlignment_T = std::alignment_of<std::max_align_t>::value>
    class PagingObjectPool
    {
    public:
        static inline constexpr uint32_t c_PageSize = static_cast<uint32_t>(PageSize_T);
        inline static constexpr uint32_t c_InvalidPageIndex = 0x0FFFFFFF;  // 28-bit max value
        inline static constexpr uint32_t c_TailPageIndex = 0x0FFFFFFE;    // 28-bit sentinel
        inline static constexpr uint32_t c_SwappingPageIndex = 0x0FFFFFFD; // Sentinel for double-push prevention
        inline static constexpr uint32_t c_MaxPages = c_TailPageIndex - 1;

        static inline constexpr uint64_t c_PageAlignment = PageAlignment_T;

        // Bit layout for 64-bit free list head (ABA-safe)
        inline static constexpr uint64_t c_bitIndexHeadPageIndex = 0;
        inline static constexpr uint64_t c_bitIndexHeadNextIndex = 28;
        inline static constexpr uint64_t c_bitIndexHeadCounter = 56;
        inline static constexpr uint64_t c_headPageIndexMask = (0x0FFFFFFFull << c_bitIndexHeadPageIndex);  // 28 bits
        inline static constexpr uint64_t c_headNextIndexMask = (0x0FFFFFFFull << c_bitIndexHeadNextIndex);  // 28 bits
        inline static constexpr uint64_t c_headCounterMask = (0xFFull << c_bitIndexHeadCounter);            // 8 bits

        inline static constexpr uint64_t c_emptyFreeListHeadIndex = ((static_cast<uint64_t>(c_TailPageIndex) << c_bitIndexHeadNextIndex) | static_cast<uint64_t>(c_TailPageIndex));
    private:
        struct Page;
        struct Node
        {
            T data;
            uint32_t pageIndex = 0;

            template<typename... Args>
            Node(Args&&... args) : data(std::forward<Args>(args)...)
            {
            }
        };

        using ThisPoolType = PagingObjectPool<T, PageSize_T, PageAlignment_T>;
        using FixedSizeObjectPoolType = FixedSizeObjectPool<Node, PageSize_T, PageAlignment_T>;

        struct Page
        {
            FixedSizeObjectPoolType data;
            uint32_t pageIndex = 0;
            uint32_t nextFreeIndex = c_InvalidPageIndex;
        };

        CoreTypes::CountingSpinlock pageListLock;

        Page** pPageList = nullptr;
        uint32_t numPages = 0;
        uint32_t pageCapacity = 0;

        PKLE_DECLARE_ATOMIC_ALIGNED(uint32_t, count) = 0;
        
        // Lock-free singly-linked list head with ABA counter (packed 64-bit)
        uint64_t freeListHeadIndex = c_emptyFreeListHeadIndex;

        void PushPageToFreeList(Page* pPage)
        {
            if(pPage)
            {
                // First, prevent double-push by atomically marking this page as being pushed
                bool bSetNextIndex = false;
                while(pPage->nextFreeIndex == c_InvalidPageIndex && !bSetNextIndex)
                {
                    bSetNextIndex = Util::AtomicCompareExchange<uint32_t>(pPage->nextFreeIndex, c_SwappingPageIndex, c_InvalidPageIndex, Util::MemoryOrder::ACQUIRE, Util::MemoryOrder::RELAXED);
                } 

                if(bSetNextIndex)
                {
                    bool bPushed = false;
                    do
                    {
                        const uint64_t currFreeListHeadIndex = Util::AtomicLoadU64(freeListHeadIndex, Util::MemoryOrder::ACQUIRE);
                        const uint32_t currFreeListHeadPageIndex = static_cast<uint32_t>((currFreeListHeadIndex & c_headPageIndexMask) >> c_bitIndexHeadPageIndex);
                        const uint32_t currFreeListHeadNextIndex = static_cast<uint32_t>((currFreeListHeadIndex & c_headNextIndexMask) >> c_bitIndexHeadNextIndex);
                        const uint8_t currCounter = static_cast<uint8_t>((currFreeListHeadIndex & c_headCounterMask) >> c_bitIndexHeadCounter);

                        const uint32_t nextHeadPageIndex = pPage->pageIndex;
                        const uint32_t nextHeadNextIndex = currFreeListHeadPageIndex;
                        const uint8_t nextCounter = static_cast<uint8_t>(currCounter + 1);  // Rolling counter to prevent ABA

                        if((nextHeadPageIndex < numPages) && ((nextHeadNextIndex < numPages) || (nextHeadNextIndex == c_TailPageIndex)))
                        {
                            const uint64_t nextFreeListHeadIndex = ((static_cast<uint64_t>(nextCounter) << c_bitIndexHeadCounter) | (static_cast<uint64_t>(nextHeadNextIndex) << c_bitIndexHeadNextIndex) | static_cast<uint64_t>(nextHeadPageIndex));
                            Util::AtomicStoreU32(pPage->nextFreeIndex, currFreeListHeadPageIndex, Util::MemoryOrder::RELEASE);
                            bPushed = Util::AtomicCompareExchangeU64(freeListHeadIndex, nextFreeListHeadIndex, currFreeListHeadIndex, Util::MemoryOrder::RELEASE, Util::MemoryOrder::RELAXED);
                        }
                        else
                        {
                            PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "PagingObjectPool::PushPageToFreeList - Invalid free list head indices");
                        }
                    } while (!bPushed);
                }
            }
        }

        Page* PopPageFromFreeList()
        {
            bool bPopped = false;
            uint32_t poppedPageIndex = c_InvalidPageIndex;
            while (!bPopped)
            {
                const uint64_t currFreeListHeadIndex = Util::AtomicLoadU64(freeListHeadIndex, Util::MemoryOrder::ACQUIRE);
                const uint32_t currFreeListHeadPageIndex = static_cast<uint32_t>((currFreeListHeadIndex & c_headPageIndexMask) >> c_bitIndexHeadPageIndex);
                const uint32_t currFreeListHeadNextIndex = static_cast<uint32_t>((currFreeListHeadIndex & c_headNextIndexMask) >> c_bitIndexHeadNextIndex);
                const uint8_t currCounter = static_cast<uint8_t>((currFreeListHeadIndex & c_headCounterMask) >> c_bitIndexHeadCounter);

                if(currFreeListHeadPageIndex < numPages)
                {
                    Page* pNextPage = nullptr;
                    if(currFreeListHeadNextIndex < numPages)
                    {
                        CoreTypes::ScopedReadSpinLock readLock(pageListLock);
                        pNextPage = pPageList[currFreeListHeadNextIndex];
                    }
                    
                    const uint32_t nextHeadPageIndex = currFreeListHeadNextIndex;
                    const uint32_t nextHeadNextIndex = (pNextPage) ? (Util::AtomicLoadU32(pNextPage->nextFreeIndex, Util::MemoryOrder::ACQUIRE)) : (c_TailPageIndex);
                    const uint8_t nextCounter = static_cast<uint8_t>(currCounter + 1);  // Rolling counter to prevent ABA

                    if(nextHeadNextIndex != c_InvalidPageIndex && nextHeadNextIndex != c_SwappingPageIndex)
                    {
                        const uint64_t nextFreeListHeadIndex = ((static_cast<uint64_t>(nextCounter) << c_bitIndexHeadCounter) | (static_cast<uint64_t>(nextHeadNextIndex) << c_bitIndexHeadNextIndex) | static_cast<uint64_t>(nextHeadPageIndex));
                        bPopped = Util::AtomicCompareExchangeU64(freeListHeadIndex, nextFreeListHeadIndex, currFreeListHeadIndex, Util::MemoryOrder::ACQUIRE, Util::MemoryOrder::RELAXED);
                    }

                    if(bPopped)
                    {
                        poppedPageIndex = currFreeListHeadPageIndex;
                    }
                }
                else if(currFreeListHeadPageIndex == c_TailPageIndex)
                {
                    // Free list is empty
                    poppedPageIndex = c_TailPageIndex;
                    bPopped = true;
                }
                else
                {
                    PKLE_ASSERT_SYSTEM_ERROR_MSG(false, "PagingObjectPool::PopPageFromFreeList - Invalid free list head page index");
                }
            }

            Page* pPoppedPage = nullptr;
            if(poppedPageIndex < numPages)
            {
                CoreTypes::ScopedReadSpinLock readLock(pageListLock);
                pPoppedPage = pPageList[poppedPageIndex];
                Util::AtomicStoreU32(pPoppedPage->nextFreeIndex, c_InvalidPageIndex, Util::MemoryOrder::RELEASE);
            }

            return pPoppedPage;
        }

        Page* AllocateNewPage()
        {
            Page* pNewPage = Util::NewAligned<Page>(c_PageAlignment);
            {
                CoreTypes::ScopedReadSpinLock readLock(pageListLock);

                uint32_t newNumPages = Util::AtomicIncrement<uint32_t>(numPages);
                if(newNumPages > pageCapacity)
                {
                    CoreTypes::ScopedWriteSpinLock writeLock(std::move(readLock));
                    if(newNumPages > pageCapacity)
                    {
                        static const uint32_t c_initialPageCapacity = 4;
                        uint32_t newPageCapacity = (pageCapacity == 0) ? (c_initialPageCapacity) : (pageCapacity * 2);
                        if(newPageCapacity < newNumPages)
                        {
                            newPageCapacity = (newNumPages * 2);
                        }

                        Page** pNewPageList = reinterpret_cast<Page**>(Util::Malloc(sizeof(Page*) * newPageCapacity));
                        if(pPageList)
                        {
                            Util::MemSet(reinterpret_cast<uint8_t*>(pNewPageList), static_cast<uint8_t>(0), sizeof(Page*) * newPageCapacity);
                            Util::MemCpy(pNewPageList, pPageList, sizeof(Page*) * pageCapacity);
                            Util::Free(pPageList);
                        }
                        pPageList = pNewPageList;
                        pageCapacity = newPageCapacity;
                    }
                    readLock = std::move(writeLock);
                }

                uint32_t newPageIndex = newNumPages - 1;
                pNewPage->pageIndex = newPageIndex;
                pPageList[newPageIndex] = pNewPage;
            }

            if(pNewPage)
            {
                PushPageToFreeList(pNewPage);
            }
            else
            {
                PKLE_ASSERT_MSG(false, "Failed to allocate new page");
            }

            return pNewPage;
        }

    public:
        struct Iterator
        {
            using PageIterator = typename FixedSizeObjectPoolType::Iterator;
            uint32_t pageIndex;
            PagingObjectPool* pPool;
            PageIterator currPageIterator;
            PageIterator endPageIterator;

            Iterator()
            {
                pageIndex = 0;
                pPool = nullptr;
            }

            Iterator(PagingObjectPool* pPool, uint32_t pageIndex)
            {
                this->pPool = pPool;
                if(pageIndex < pPool->numPages)
                {
                    Page* pPage = pPool->pPageList[pageIndex];
                    currPageIterator = pPage->data.begin();
                    endPageIterator = pPage->data.end();
                    
                    while(currPageIterator == endPageIterator)
                    {
                        if(pageIndex < (pPool->numPages - 1))
                        {
                            ++pageIndex;
                            Page* pPage = pPool->pPageList[pageIndex];
                            currPageIterator = pPage->data.begin();
                            endPageIterator = pPage->data.end();
                        }
                        else
                        {
                            //Reached the end.
                            break;
                        }
                    }
                }
                else if(pPool->numPages == 0)
                {
                    pageIndex = 0;
                }
                else
                {
                    pageIndex = pPool->numPages - 1;
                    Page* pPage = pPool->pPageList[pageIndex];
                    endPageIterator = pPage->data.end();
                    currPageIterator = endPageIterator;
                }
                this->pageIndex = pageIndex;
            }

            T* operator*()
            {
                Node* pObject = *currPageIterator;
                return &(pObject->data);
            }

            const T* operator*() const
            {
                const Node* pObject = *currPageIterator;
                return &(pObject->data);
            }

            Iterator& operator++()
            {
                ++currPageIterator;
                while(currPageIterator == endPageIterator)
                {
                    if(pageIndex < (pPool->numPages - 1))
                    {
                        ++pageIndex;
                        Page* pPage = pPool->pPageList[pageIndex];
                        currPageIterator = pPage->data.begin();
                        endPageIterator = pPage->data.end();
                    }
                    else
                    {
                        //Reached the end.
                        break;
                    }
                }
                return *this;
            }

            bool operator!=(const Iterator& other) const
            {
                const bool bResult = (pageIndex != other.pageIndex) || (currPageIterator != other.currPageIterator);
                return bResult;
            }

            bool operator==(const Iterator& other) const
            {
                const bool bResult = (pageIndex == other.pageIndex) && (currPageIterator == other.currPageIterator);
                return bResult;
            }
        };

        Iterator begin()
        {
            Iterator it(this, 0);
            return it;
        }

        Iterator end()
        {
            if(numPages == 0)
            {
                return begin();
            }
            Iterator it(this, numPages - 1);
            it.currPageIterator = it.endPageIterator;
            return it;
        }


        PagingObjectPool()
        {
            // AllocateNewPage();
        }

        ~PagingObjectPool()
        {
            if(pPageList)
            {
                for(uint32_t i = 0; i < numPages; ++i)
                {
                    Util::DeleteAligned<Page>(pPageList[i]);
                }
                Util::Free(pPageList);
                pPageList = nullptr;
            }
        }

        void PreallocateSpace(uint32_t numObjects)
        {
            uint32_t numPagesNeeded = (numObjects + c_PageSize - 1) / c_PageSize;
            for(uint32_t i = 0; i < numPagesNeeded; ++i)
            {
                AllocateNewPage();
            }
        }
        
        template<typename... Args_T>
        T* Reserve(Args_T... args)
        {
            Node* pSelectedNode = nullptr;
            do
            {
                Page* pPageWithSpace = PopPageFromFreeList();
                if(pPageWithSpace)
                {
                    pSelectedNode = pPageWithSpace->data.Reserve(args...);
                    if(pSelectedNode)
                    {
                        pSelectedNode->pageIndex = pPageWithSpace->pageIndex;
                        Util::AtomicIncrementU32(count);
                        //We allocated from the page, so re-add it to the free space list if it's not full
                        if(!pPageWithSpace->data.IsFull())
                        {
                            PushPageToFreeList(pPageWithSpace);
                        }
                    }
                }
                else
                {
                    AllocateNewPage();
                }
            } while (pSelectedNode == nullptr);

            T* pObject = nullptr;
            if(pSelectedNode)
            {
                pObject = &(pSelectedNode->data);
            }

            PKLE_ASSERT_SYSTEM_ERROR_MSG(pObject != nullptr, "PagingObjectPool::Reserve: Failed to reserve object from pool.");

            return pObject;
        }

        void* ReserveRaw()
        {
            Node* pSelectedNode = nullptr;
            do
            {
                Page* pPageWithSpace = PopPageFromFreeList();
                if(pPageWithSpace)
                {
                    void* pReservedRawNode = pPageWithSpace->data.ReserveRaw();
                    if(pReservedRawNode)
                    {
                        pSelectedNode = reinterpret_cast<Node*>(pReservedRawNode);
                        pSelectedNode->pageIndex = pPageWithSpace->pageIndex;

                        Util::AtomicIncrementU32(count);
                        //We allocated from the page, so re-add it to the free space list if it's not full
                        if(!pPageWithSpace->data.IsFull())
                        {
                            PushPageToFreeList(pPageWithSpace);
                        }
                    }
                }
                else
                {
                    AllocateNewPage();
                }
            } while (pSelectedNode == nullptr);

            void* pRawMemory = nullptr;
            if(pSelectedNode)
            {
                pRawMemory = reinterpret_cast<void*>(&(pSelectedNode->data));
            }

            PKLE_ASSERT_SYSTEM_ERROR_MSG(pRawMemory != nullptr, "PagingObjectPool::ReserveRaw: Failed to reserve raw memory from pool.");

            return pRawMemory;
        }

        bool Release(const T* pObject)
        {
            bool bReleased = false;
            const Node* pNode = reinterpret_cast<const Node*>(pObject);
            if(pNode)
            {
                uint32_t pageIndex = pNode->pageIndex;
                if(pageIndex < numPages)
                {
                    Page* pPage = nullptr;
                    {
                        CoreTypes::ScopedReadSpinLock readLock(pageListLock);
                        pPage = pPageList[pageIndex];
                    }

                    bReleased = pPage->data.Release(pNode);
                    if(bReleased)
                    {
                        Util::AtomicDecrementU32(count);
                    }

                    // Page has space, so try to add it back to the free space list
                    PushPageToFreeList(pPage);
                }
            }
            return bReleased;
        }

        bool ReleaseRaw(const void* pObject)
        {
            bool bReleased = false;
            const Node* pNode = reinterpret_cast<const Node*>(pObject);
            if(pNode)
            {
                uint32_t pageIndex = pNode->pageIndex;
                if(pageIndex < numPages)
                {
                    Page* pPage = nullptr;
                    {
                        CoreTypes::ScopedReadSpinLock readLock(pageListLock);
                        pPage = pPageList[pageIndex];
                    }

                    bReleased = pPage->data.ReleaseRaw(pNode);
                    if(bReleased)
                    {
                        Util::AtomicDecrementU32(count);
                    }

                    // Page has space, so try to add it back to the free space list
                    PushPageToFreeList(pPage);
                }
            }
            return bReleased;
        }

        uint32_t GetCapacity() const
        {
            return numPages * PageSize_T;
        }

        void Clear()
        {
            if(pPageList)
            {
                for(uint32_t i = 0; i < numPages; ++i)
                {
                    Util::DeleteAligned<Page>(pPageList[i]);
                }
                Util::Free(pPageList);
                pPageList = nullptr;
            }
            
            numPages = 0;
            pageCapacity = 0;
            
            freeListHeadIndex = c_emptyFreeListHeadIndex;
            count = 0;
        }


        uint32_t Size() const
        {
            return count;
        }

    };


}; //end namespace CoreTypes
}; //end namespace PklE