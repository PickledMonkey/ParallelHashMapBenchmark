#pragma once

#include <stdint.h>
#include "atomic_util.h"
#include "spin_lock.h"

namespace PklE
{
namespace ThreadsafeContainers
{
    // Node_T must have the following members:
    // - Node_T* pNext = nullptr; (pointer to next node)
    // - A key member that can be compared for equality
    //
    // This is a simple unsorted linked list where:
    // - New nodes are always inserted at the front
    // - Lookups and inserts use read locks with atomic CAS for insertion
    // - Erases use write locks and return the disconnected node pointer
    // - External systems handle node allocation/deallocation
    template<typename Node_T>
    class SimpleLinkedList
    {
    public:
        using ThisLinkedListType = SimpleLinkedList<Node_T>;

    private:
        PKLE_DECLARE_ATOMIC_ALIGNED(Node_T*, pHead) = nullptr;
        CoreTypes::CountingSpinlock lock;

        // Compare-and-swap wrapper for pointer operations
        bool CAS(Node_T** location, Node_T* expected, Node_T* desired)
        {
            return PklE::Util::AtomicCompareExchangePtr(
                reinterpret_cast<void**>(location), 
                reinterpret_cast<void*>(desired), 
                reinterpret_cast<void*>(expected)
            );
        }

    public:
        // Constructor
        SimpleLinkedList() = default;

        // Destructor - does NOT delete nodes, external system manages memory
        ~SimpleLinkedList()
        {
            // Nodes are managed externally, so we just reset the head
            pHead = nullptr;
        }

        const Node_T* GetHead() const
        {
            return pHead;
        }

        Node_T* GetHead()
        {
            return pHead;
        }

        // Insert a new node at the front of the list
        // Uses read lock with atomic CAS for concurrent inserts
        // Returns true on success, false on failure
        bool Insert(Node_T* newNode)
        {
            if (!newNode)
            {
                return false;
            }

            CoreTypes::ScopedReadSpinLock readLock(lock);

            // Try to insert at the front using CAS
            // This allows multiple concurrent inserts with just a read lock
            do
            {
                Node_T* currentHead = pHead;
                newNode->pNext = currentHead;
                
                if (CAS(&pHead, currentHead, newNode))
                {
                    return true;
                }
                // CAS failed, retry with new head value
            } while (true);

            return false; // Should never reach here
        }

        // Insert a new node at the front of the list without locking
        // WARNING: Only use when external synchronization guarantees exclusive access
        bool Insert_Unsafe(Node_T* newNode)
        {
            if (!newNode)
            {
                return false;
            }

            // No locking, directly insert at front
            Node_T* currentHead = pHead;
            newNode->pNext = currentHead;
            pHead = newNode;

            return true;
        }

        // Find a node with the given key
        // Uses read lock for thread-safe traversal
        // Returns pointer to found node, or nullptr if not found
        template<typename Key_T, typename Predicate_T>
        Node_T* Find(const Key_T& searchKey)
        {
            CoreTypes::ScopedReadSpinLock readLock(lock);

            Node_T* current = pHead;
            while (current != nullptr)
            {
                if (Predicate_T::Compare(current->key, searchKey) == 0)
                {
                    return current;
                }
                current = current->pNext;
            }

            return nullptr;
        }

        // Erase a node with the given key
        // Uses write lock to safely unlink the node
        // Returns pointer to the erased node (caller must handle deallocation)
        // Returns nullptr if node was not found
        template<typename Key_T, typename Predicate_T>
        Node_T* Erase(const Key_T& searchKey)
        {
            CoreTypes::ScopedWriteSpinLock writeLock(lock);

            Node_T* current = pHead;
            Node_T* prev = nullptr;

            while (current != nullptr)
            {
                if (Predicate_T::Compare(current->key, searchKey) == 0)
                {
                    // Found the node, unlink it
                    if (prev == nullptr)
                    {
                        // Removing head node
                        pHead = current->pNext;
                    }
                    else
                    {
                        // Removing middle/tail node
                        prev->pNext = current->pNext;
                    }

                    // Clear the next pointer of the removed node
                    current->pNext = nullptr;
                    
                    // Return the unlinked node for external cleanup
                    return current;
                }

                prev = current;
                current = current->pNext;
            }

            return nullptr; // Not found
        }

        // Erase a specific node by pointer comparison
        // Uses write lock to safely unlink the node
        // Returns pointer to the erased node (caller must handle deallocation)
        // Returns nullptr if node was not found
        Node_T* EraseNode(const Node_T* nodeToRemove)
        {
            if (!nodeToRemove)
            {
                return nullptr;
            }

            CoreTypes::ScopedWriteSpinLock writeLock(lock);

            Node_T* current = pHead;
            Node_T* prev = nullptr;

            while (current != nullptr)
            {
                if (current == nodeToRemove)
                {
                    // Found the node, unlink it
                    if (prev == nullptr)
                    {
                        // Removing head node
                        pHead = current->pNext;
                    }
                    else
                    {
                        // Removing middle/tail node
                        prev->pNext = current->pNext;
                    }

                    // Clear the next pointer of the removed node
                    current->pNext = nullptr;
                    
                    // Return the unlinked node for external cleanup
                    return current;
                }

                prev = current;
                current = current->pNext;
            }

            return nullptr; // Not found
        }

        // Find a node with the given key without locking
        // WARNING: Only use when external synchronization guarantees safe access
        template<typename Key_T, typename Predicate_T>
        Node_T* Find_Unsafe(const Key_T& searchKey)
        {
            Node_T* current = pHead;
            while (current != nullptr)
            {
                if (Predicate_T::Compare(current->key, searchKey) == 0)
                {
                    return current;
                }
                current = current->pNext;
            }

            return nullptr;
        }

        // Find the last node with the given key
        // Uses read lock for thread-safe traversal
        // Returns pointer to the last matching node, or nullptr if not found
        template<typename Key_T, typename Predicate_T>
        Node_T* FindLast(const Key_T& searchKey)
        {
            CoreTypes::ScopedReadSpinLock readLock(lock);

            Node_T* current = pHead;
            Node_T* lastMatch = nullptr;
            
            while (current != nullptr)
            {
                if (Predicate_T::Compare(current->key, searchKey) == 0)
                {
                    lastMatch = current;
                }
                current = current->pNext;
            }

            return lastMatch;
        }

        // Find the last node with the given key without locking
        // WARNING: Only use when external synchronization guarantees safe access
        template<typename Key_T, typename Predicate_T>
        Node_T* FindLast_Unsafe(const Key_T& searchKey)
        {
            Node_T* current = pHead;
            Node_T* lastMatch = nullptr;
            
            while (current != nullptr)
            {
                if (Predicate_T::Compare(current->key, searchKey) == 0)
                {
                    lastMatch = current;
                }
                current = current->pNext;
            }

            return lastMatch;
        }

        // Erase a node with the given key without locking
        // WARNING: Only use when external synchronization guarantees exclusive access
        // Returns pointer to the erased node (caller must handle deallocation)
        // Returns nullptr if node was not found
        template<typename Key_T, typename Predicate_T>
        Node_T* Erase_Unsafe(const Key_T& searchKey)
        {
            Node_T* current = pHead;
            Node_T* prev = nullptr;

            while (current != nullptr)
            {
                if (Predicate_T::Compare(current->key, searchKey) == 0)
                {
                    // Found the node, unlink it
                    if (prev == nullptr)
                    {
                        // Removing head node
                        pHead = current->pNext;
                    }
                    else
                    {
                        // Removing middle/tail node
                        prev->pNext = current->pNext;
                    }

                    // Clear the next pointer of the removed node
                    current->pNext = nullptr;
                    
                    // Return the unlinked node for external cleanup
                    return current;
                }

                prev = current;
                current = current->pNext;
            }

            return nullptr; // Not found
        }

        // Erase a specific node by pointer comparison without locking
        // WARNING: Only use when external synchronization guarantees exclusive access
        // Returns pointer to the erased node (caller must handle deallocation)
        // Returns nullptr if node was not found
        Node_T* EraseNode_Unsafe(const Node_T* nodeToRemove)
        {
            if (!nodeToRemove)
            {
                return nullptr;
            }

            Node_T* current = pHead;
            Node_T* prev = nullptr;

            while (current != nullptr)
            {
                if (current == nodeToRemove)
                {
                    // Found the node, unlink it
                    if (prev == nullptr)
                    {
                        // Removing head node
                        pHead = current->pNext;
                    }
                    else
                    {
                        // Removing middle/tail node
                        prev->pNext = current->pNext;
                    }

                    // Clear the next pointer of the removed node
                    current->pNext = nullptr;
                    
                    // Return the unlinked node for external cleanup
                    return current;
                }

                prev = current;
                current = current->pNext;
            }

            return nullptr; // Not found
        }

        template<typename Key_T, typename Predicate_T>
        bool InsertUnique(Node_T* newNode)
        {
            bool bInserted = false;
            if (newNode)
            {
                CoreTypes::ScopedWriteSpinLock writeLock(lock);

                // Check for existing node with the same key
                Node_T* existingNode = Find_Unsafe<Key_T, Predicate_T>(newNode->key);
                if (!existingNode)
                {
                    // No existing node, safe to insert
                    Node_T* currentHead = pHead;
                    newNode->pNext = currentHead;
                    pHead = newNode;
                    bInserted = true;
                }
            }
            return bInserted;
        }

        template<typename Key_T, typename Predicate_T>
        bool InsertUnique_Unsafe(Node_T* newNode)
        {
            bool bInserted = false;
            if (newNode)
            {
                // Check for existing node with the same key
                Node_T* existingNode = Find_Unsafe<Key_T, Predicate_T>(newNode->key);
                if (!existingNode)
                {
                    // No existing node, safe to insert
                    Node_T* currentHead = pHead;
                    newNode->pNext = currentHead;
                    pHead = newNode;
                    bInserted = true;
                }
            }
            return bInserted;
        }

        // Check if the list is empty
        bool IsEmpty() const
        {
            return pHead == nullptr;
        }

        // Reset the list without deallocating nodes
        // WARNING: Only use when external synchronization guarantees exclusive access
        void Reset_Unsafe()
        {
            pHead = nullptr;
        }

        // Get the head pointer (for iteration or debugging)
        // WARNING: Use with caution in concurrent scenarios
        Node_T* GetHead_Unsafe() const
        {
            return pHead;
        }
    };

} // end namespace ThreadsafeContainers
} // end namespace PklE
