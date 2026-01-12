#include "spin_lock.h"

#include <thread>
#include "atomic_util.h"
#include "logging_util.h"

namespace PklE
{
namespace CoreTypes
{
    CountingSpinlock::CountingSpinlock() : lockValue(0)
    {
    }

    CountingSpinlock::CountingSpinlock(CountingSpinlock&& other) 
    {
        lockValue = other.lockValue;
        other.lockValue = 0;
    }

    CountingSpinlock& CountingSpinlock::operator=(CountingSpinlock&& other)
    {
        lockValue = other.lockValue;
        other.lockValue = 0;
        return *this;
    }

    void CountingSpinlock::AcquireReadOnlyAccess()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            uint32_t nextVal = Util::AtomicIncrementU32(lockValue);
            if ((nextVal & c_multiReaderWriter_WriteMask) == 0)
            {
                //No write lock is held, so we can proceed
                break;
            }
            else
            {
                //A write lock is held, so we need to wait for it to be released
                while ((lockValue & c_multiReaderWriter_WriteMask) != 0)
                {
                    //Spin until the write lock is released
                    std::this_thread::yield();
                }
                break; //We got it, so break out.
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::AcquireReadOnlyAccess - Failed to acquire read lock after maximum retries");
    }
    void CountingSpinlock::ReleaseReadOnlyAccess()
    {
        Util::AtomicDecrementU32(lockValue);
    }

    void CountingSpinlock::AcquireReadAndWriteAccess()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do 
        {
            uint32_t nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
            if (nextVal == c_multiReaderWriter_WriteIncrement)
            {
                // No read or write locks are held, so we can proceed
                break;
            }
            else 
            {
                // Undo the write-lock increment
                nextVal = Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);
                if(nextVal != 0)
                {
                    // A read or write lock is held, so we need to wait for them to be released
                    while (lockValue != 0)
                    {
                        // Spin until the read and write locks are released
                        std::this_thread::yield();
                    }
                }

                // Try grabbing it again since there aren't any read or write locks held now
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::AcquireReadAndWriteAccess - Failed to acquire write lock after maximum retries");
    }

    void CountingSpinlock::ReleaseReadAndWriteAccess()
    {
        Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);
    }

    void CountingSpinlock::ConvertFromReadToWriteLock()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            uint32_t nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
            if((nextVal & c_multiReaderWriter_WriteMask) == c_multiReaderWriter_WriteIncrement)
            {
                //We have the first write lock.

                //Undo the read-lock increment
                nextVal = Util::AtomicDecrementU32(lockValue);
                if(nextVal == c_multiReaderWriter_WriteIncrement)
                {
                    //No read locks are held, so we can proceed
                    break;
                }
                else
                {
                    //There are still read locks held by other threads.

                    //Read-locks are prioritized, so undo our write-lock increment
                    nextVal = Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);
                    if(nextVal != 0)
                    {
                        //Wait for the read locks to be released
                        while(lockValue != 0)
                        {
                            //Spin until the read and write locks are released
                            std::this_thread::yield();
                        }
                    }

                    // Try grabbing it again since there aren't any read or write locks held now
                    AcquireReadAndWriteAccess();

                    break;
                }
            }
            else
            {
                //We didn't get the first write lock, so undo the write-lock increment
                nextVal = Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

                //Undo the read-lock increment
                nextVal = Util::AtomicDecrementU32(lockValue);
                if(nextVal != 0)
                {
                    while(lockValue != 0)
                    {
                        //Spin until the read and write locks are released
                        std::this_thread::yield();
                    }
                }

                // Try grabbing it again since there aren't any read or write locks held now
                AcquireReadAndWriteAccess();
                break;
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::ConvertFromReadToWriteLock - Failed to convert read lock to write lock after maximum retries");
    }

    void CountingSpinlock::ConvertFromWriteToReadLock()
    {
        // Acquire a read lock by incrementing the read count
        uint32_t nextVal = Util::AtomicIncrementU32(lockValue);

        // Release the write lock by decrementing the write count
        nextVal = Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

        // At this point, we should have a read lock held and no write locks held because read locks are priority over write locks
        // But just in case, we will wait for any write locks to be released before proceeding
        if(nextVal & c_multiReaderWriter_WriteMask)
        {    
            while(lockValue & c_multiReaderWriter_WriteMask)
            {
                //Spin until the write locks are released
                std::this_thread::yield();
            }
        }
    }

    void CountingSpinlock::AcquireWritePriorityReadOnlyAccess()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            uint32_t nextVal = Util::AtomicIncrementU32(lockValue);
            if ((nextVal & c_multiReaderWriter_WriteMask) == 0)
            {
                //No write lock is held, so we can proceed
                break;
            }
            else
            {
                //Undo the read-lock increment
                Util::AtomicDecrementU32(lockValue);

                //A write lock is held, so we need to wait for it to be released
                while ((lockValue & c_multiReaderWriter_WriteMask) != 0)
                {
                    //Spin until the write lock is released
                    std::this_thread::yield();
                }
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::AcquireWritePriorityReadOnlyAccess - Failed to acquire read lock after maximum retries");
    }
    void CountingSpinlock::ReleaseWritePriorityReadOnlyAccess()
    {
        Util::AtomicDecrementU32(lockValue);
    }

    void CountingSpinlock::AcquireWritePriorityReadAndWriteAccess()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do 
        {
            uint32_t nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
            if (nextVal == c_multiReaderWriter_WriteIncrement)
            {
                //No read or write locks are held, so we can proceed
                break;
            }
            else if ((nextVal & c_multiReaderWriter_WriteMask) > c_multiReaderWriter_WriteIncrement)
            {
                //A write-lock is held, so we need to wait for it to be released
                //Undo the write-lock increment
                Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

                //No write lock is held, but there are read locks held, so we need to wait for them to be released
                while ((lockValue & c_multiReaderWriter_WriteMask) > c_multiReaderWriter_WriteIncrement)
                {
                    //Spin until the write locks are released
                    std::this_thread::yield();
                }
            }
            else //If we're here, it means we grabbed the write lock, but there are read locks held
            {
                //Wait for the read-locks to be released
                while ((lockValue & c_multiReaderWriter_ReadMask) != 0)
                {
                    //Spin until the read locks are released
                    std::this_thread::yield();
                }

                //Read locks are release, and we have the write lock, so we can proceed
                break;
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::AcquireWritePriorityReadAndWriteAccess - Failed to acquire write lock after maximum retries");
    }

    void CountingSpinlock::ReleaseWritePriorityReadAndWriteAccess()
    {
        Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);
    }

    void CountingSpinlock::ConvertFromWritePriorityReadToWriteLock()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            //Start by attempting to grab the write lock
            uint32_t nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
            if((nextVal & c_multiReaderWriter_WriteMask) == c_multiReaderWriter_WriteIncrement)
            {
                //Nobody else has a write lock held, so we have it.

                //Now release our read lock
                nextVal = Util::AtomicDecrementU32(lockValue);
                if((nextVal & c_multiReaderWriter_ReadMask) == 0)
                {
                    //No read locks are held, so we can proceed
                    break;
                }
                else
                {
                    //There are still read locks held by other threads. We need to wait for them to be released

                    //Wait for all read locks to be released
                    while ((lockValue & c_multiReaderWriter_ReadMask) != 0)
                    {
                        //Spin until the read locks are released
                        std::this_thread::yield();
                    }

                    //We have the write lock and there are no read locks held, so we can proceed
                    break;
                }
            }
            else
            {
                //Undo the write-lock increment since we couldn't grab it
                Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

                //Undo our read-lock increment since we couldn't grab the write lock
                Util::AtomicDecrementU32(lockValue);

                //Attempt to grab the write lock using the standard method since we no longer have our read lock
                AcquireWritePriorityReadAndWriteAccess();
                break;
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::ConvertFromWritePriorityReadToWriteLock - Failed to convert read lock to write lock after maximum retries");
    }

    void CountingSpinlock::ConvertFromWritePriorityWriteToReadLock()
    {
        //Acquire a read lock by incrementing the read count. Because this is a write-priority lock, we should be the only ones holding a write lock
        uint32_t nextVal = Util::AtomicIncrementU32(lockValue);
        
        //Release our write lock, and we're now able to proceed with just the read lock held
        nextVal = Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

        if ((nextVal & c_multiReaderWriter_WriteMask) == 0)
        {
            //No other write locks are held, so we can proceed
        }
        else
        {
            //There are still write locks in other threads.
            //Give up the read lock we just acquired
            Util::AtomicDecrementU32(lockValue);

            //Acquire the read lock again the normal way
            AcquireWritePriorityReadOnlyAccess();
        }
    }

    void CountingSpinlock::AcquireMultiReaderWriterReadAccess()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            uint32_t nextVal = Util::AtomicIncrementU32(lockValue);
            if ((nextVal & c_multiReaderWriter_WriteMask) == 0)
            {
                //No write locks are held, so we can proceed
                break;
            }
            else
            {
                //A write lock is held, so we need to wait for it to be released
                while ((lockValue & c_multiReaderWriter_WriteMask) != 0)
                {
                    //Spin until the write lock is released
                    std::this_thread::yield();
                }

                break; //No write locks are held, so we can proceed
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::AcquireMultiReaderWriterReadAccess - Failed to acquire read lock after maximum retries");
    }
    
    void CountingSpinlock::ReleaseMultiReaderWriterReadAccess()
    {
        Util::AtomicDecrementU32(lockValue);
    }

    void CountingSpinlock::AcquireMultiReaderWriterWriteAccess()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            uint32_t nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
            if ((nextVal & c_multiReaderWriter_ReadMask) == 0)
            {
                //No read locks are held, so we can proceed
                break;
            }
            else
            {
                //Undo the write-lock increment
                Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

                //A read lock is held, so we need to wait for it to be released
                while ((lockValue & c_multiReaderWriter_ReadMask) != 0)
                {
                    //Spin until the read lock is released
                    std::this_thread::yield();
                }
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::AcquireMultiReaderWriterWriteAccess - Failed to acquire write lock after maximum retries");
    }
    
    void CountingSpinlock::ReleaseMultiReaderWriterWriteAccess()
    {
        Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);
    }

    void CountingSpinlock::ConvertFromMultiReaderWriterReadToWriteLock()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;
        do
        {
            //Increment the write lock count, so the write-lock is already held when we release the read-lock
            uint32_t nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
            nextVal = Util::AtomicDecrementU32(lockValue);
            
            if ((nextVal & c_multiReaderWriter_ReadMask) == 0)
            {
                //No read locks are held, so we can proceed
                break;
            }
            else
            {
                do
                {
                    //Undo the write-lock increment that we held before we grabbed the write lock so that we don't block other readers
                    Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);

                    //A read lock is held, so we need to wait for it to be released
                    while ((lockValue & c_multiReaderWriter_ReadMask) != 0)
                    {
                        //Spin until the read lock is released
                        std::this_thread::yield();
                    }

                    //Try to grab the write lock again
                    nextVal = Util::AtomicAddU32(lockValue, c_multiReaderWriter_WriteIncrement);
                    if ((nextVal & c_multiReaderWriter_ReadMask) == 0)
                    {
                        //No read locks are held, so we can proceed
                        break;
                    }
                    --numRetriesLeft;
                } while(numRetriesLeft > 0);

                //The sub-loop above exited because we either got the lock or ran out of retries, so break out of the main loop as well
                break;
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::ConvertFromMultiReaderWriterReadToWriteLock - Failed to convert read lock to write lock after maximum retries");
    }

	void CountingSpinlock::ConvertFromMultiReaderWriterWriteToReadLock()
    {
        uint32_t numRetriesLeft = 0xFFFFFFFF;

        do
        {
            //Increment the read lock count, so the read-lock is already held when we release the write-lock
            uint32_t nextVal = Util::AtomicIncrementU32(lockValue);

            //Release the write lock
            nextVal = Util::AtomicSubtractU32(lockValue, c_multiReaderWriter_WriteIncrement);
            if ((nextVal & c_multiReaderWriter_WriteMask) == 0)
            {
                //No write locks are held, so we can proceed
                break;
            }
            else
            {
                //A write lock is held, so we need to wait for it to be released
                while ((lockValue & c_multiReaderWriter_WriteMask) != 0)
                {
                    //Spin until the write lock is released
                    std::this_thread::yield();
                }

                break; //No write locks are held, so we can proceed
            }
            --numRetriesLeft;
        } while (numRetriesLeft > 0);
        PKLE_ASSERT_SYSTEM_WARNING_MSG(numRetriesLeft > 0, "CountingSpinlock::ConvertFromMultiReaderWriterWriteToReadLock - Failed to convert write lock to read lock after maximum retries");
    }

    //------------------------------------------------
    // Standard read-write lock scoped implementations
    //------------------------------------------------

    ScopedReadSpinLock::ScopedReadSpinLock()
    {
    }

    ScopedReadSpinLock::ScopedReadSpinLock(CountingSpinlock& lock) : pLock(&lock)
    {
        pLock->AcquireReadOnlyAccess();
    }

    ScopedReadSpinLock::ScopedReadSpinLock(CountingSpinlock* pLock) : pLock(pLock)
    {
        if(pLock)
        {
            pLock->AcquireReadOnlyAccess();
        }
    }

    ScopedReadSpinLock::ScopedReadSpinLock(ScopedReadSpinLock&& other) : pLock(other.pLock)
    {
        other.pLock = nullptr;
    }

    ScopedReadSpinLock::~ScopedReadSpinLock()
    {
        if(pLock)
        {
            pLock->ReleaseReadOnlyAccess();
            pLock = nullptr;
        }
    }

    ScopedWriteSpinLock::ScopedWriteSpinLock()
    {
    }

    ScopedWriteSpinLock::ScopedWriteSpinLock(CountingSpinlock& lock) : pLock(&lock)
    {
        pLock->AcquireReadAndWriteAccess();
    }

    ScopedWriteSpinLock::ScopedWriteSpinLock(CountingSpinlock* pLock) : pLock(pLock)
    {
        if(pLock)
        {
            pLock->AcquireReadAndWriteAccess();
        }
    }

    ScopedWriteSpinLock::ScopedWriteSpinLock(ScopedWriteSpinLock&& other) : pLock(other.pLock)
    {
        other.pLock = nullptr;
    }

    ScopedWriteSpinLock::~ScopedWriteSpinLock()
    {
        if(pLock)
        {
            pLock->ReleaseReadAndWriteAccess();
        }
    }

    ScopedWriteSpinLock& ScopedWriteSpinLock::operator=(ScopedWriteSpinLock&& other)
    {
        if(pLock)
        {
            pLock->ReleaseReadAndWriteAccess();
            pLock = nullptr;
        }

        if(other.pLock)
        {
            pLock = other.pLock;
            other.pLock = nullptr;
        }
        return *this;
    }

    //------------------------------------------------
    // Write-priority read-write lock scoped implementations
    //------------------------------------------------

    ScopedWritePriorityReadSpinLock::ScopedWritePriorityReadSpinLock()
    {
    }

    ScopedWritePriorityReadSpinLock::ScopedWritePriorityReadSpinLock(CountingSpinlock& lock) : pLock(&lock)
    {
        pLock->AcquireWritePriorityReadOnlyAccess();
    }

    ScopedWritePriorityReadSpinLock::ScopedWritePriorityReadSpinLock(CountingSpinlock* pLock) : pLock(pLock)
    {
        if(pLock)
        {
            pLock->AcquireWritePriorityReadOnlyAccess();
        }
    }

    ScopedWritePriorityReadSpinLock::ScopedWritePriorityReadSpinLock(ScopedWritePriorityReadSpinLock&& other) : pLock(other.pLock)
    {
        other.pLock = nullptr;
    }

    ScopedWritePriorityReadSpinLock::~ScopedWritePriorityReadSpinLock()
    {
        if(pLock)
        {
            pLock->ReleaseWritePriorityReadOnlyAccess();
            pLock = nullptr;
        }
    }

    ScopedWritePriorityWriteSpinLock::ScopedWritePriorityWriteSpinLock()
    {
    }

    ScopedWritePriorityWriteSpinLock::ScopedWritePriorityWriteSpinLock(CountingSpinlock& lock) : pLock(&lock)
    {
        pLock->AcquireWritePriorityReadAndWriteAccess();
    }

    ScopedWritePriorityWriteSpinLock::ScopedWritePriorityWriteSpinLock(CountingSpinlock* pLock) : pLock(pLock)
    {
        if(pLock)
        {
            pLock->AcquireWritePriorityReadAndWriteAccess();
        }
    }

    ScopedWritePriorityWriteSpinLock::ScopedWritePriorityWriteSpinLock(ScopedWritePriorityWriteSpinLock&& other) : pLock(other.pLock)
    {
        other.pLock = nullptr;
    }

    ScopedWritePriorityWriteSpinLock::~ScopedWritePriorityWriteSpinLock()
    {
        if(pLock)
        {
            pLock->ReleaseWritePriorityReadAndWriteAccess();
        }
    }

    ScopedWritePriorityWriteSpinLock& ScopedWritePriorityWriteSpinLock::operator=(ScopedWritePriorityWriteSpinLock&& other)
    {
        if(pLock)
        {
            pLock->ReleaseWritePriorityReadAndWriteAccess();
            pLock = nullptr;
        }

        if(other.pLock)
        {
            pLock = other.pLock;
            other.pLock = nullptr;
        }
        return *this;
    }

    //------------------------------------------------
    // Multi-Reader/Writer lock scoped implementations
    //------------------------------------------------
    ScopedMultiReaderWriterReadSpinLock::ScopedMultiReaderWriterReadSpinLock()
    {
        pLock = nullptr;
    }
    
    ScopedMultiReaderWriterReadSpinLock::ScopedMultiReaderWriterReadSpinLock(CountingSpinlock& lock)
    {
        pLock = &lock;
        pLock->AcquireMultiReaderWriterReadAccess();
    }

    ScopedMultiReaderWriterReadSpinLock::ScopedMultiReaderWriterReadSpinLock(CountingSpinlock* pLock)
    {
        if(pLock)
        {
            this->pLock = pLock;
            this->pLock->AcquireMultiReaderWriterReadAccess();
        }
        else
        {
            this->pLock = nullptr;
        }
    }

    ScopedMultiReaderWriterReadSpinLock::ScopedMultiReaderWriterReadSpinLock(ScopedMultiReaderWriterReadSpinLock&& other)
    {
        pLock = other.pLock;
        other.pLock = nullptr;
    }

    ScopedMultiReaderWriterReadSpinLock::~ScopedMultiReaderWriterReadSpinLock()
    {
        if(pLock)
        {
            pLock->ReleaseMultiReaderWriterReadAccess();
            pLock = nullptr;
        }
    }

    ScopedMultiReaderWriterReadSpinLock& ScopedMultiReaderWriterReadSpinLock::operator=(ScopedMultiReaderWriterReadSpinLock&& other)
    {
        if(pLock)
        {
            pLock->ReleaseMultiReaderWriterReadAccess();
            pLock = nullptr;
        }

        pLock = other.pLock;
        other.pLock = nullptr;

        return *this;
    }

    ScopedMultiReaderWriterWriteSpinLock::ScopedMultiReaderWriterWriteSpinLock()
    {
        pLock = nullptr;
    }

    ScopedMultiReaderWriterWriteSpinLock::ScopedMultiReaderWriterWriteSpinLock(CountingSpinlock& lock)
    {
        pLock = &lock;
        pLock->AcquireMultiReaderWriterWriteAccess();
    }

    ScopedMultiReaderWriterWriteSpinLock::ScopedMultiReaderWriterWriteSpinLock(CountingSpinlock* pLock)
    {
        if(pLock)
        {
            this->pLock = pLock;
            this->pLock->AcquireMultiReaderWriterWriteAccess();
        }
        else
        {
            this->pLock = nullptr;
        }
    }

    ScopedMultiReaderWriterWriteSpinLock::ScopedMultiReaderWriterWriteSpinLock(ScopedMultiReaderWriterWriteSpinLock&& other)
    {
        pLock = other.pLock;
        other.pLock = nullptr;
    }

    ScopedMultiReaderWriterWriteSpinLock::~ScopedMultiReaderWriterWriteSpinLock()
    {
        if(pLock)
        {
            pLock->ReleaseMultiReaderWriterWriteAccess();
            pLock = nullptr;
        }
    }

    ScopedMultiReaderWriterWriteSpinLock& ScopedMultiReaderWriterWriteSpinLock::operator=(ScopedMultiReaderWriterWriteSpinLock&& other)
    {
        if(pLock)
        {
            pLock->ReleaseMultiReaderWriterWriteAccess();
            pLock = nullptr;
        }

        pLock = other.pLock;
        other.pLock = nullptr;

        return *this;
    }

    //------------------------------------------------
    // Lock transfer specializations
    //------------------------------------------------

    // Standard read-write lock transfer specializations
    template<>
	void TransferScopedLock<ScopedReadSpinLock, ScopedReadSpinLock>(ScopedReadSpinLock& toLock, ScopedReadSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseReadOnlyAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
        }
    }

	template<>
	void TransferScopedLock<ScopedReadSpinLock, ScopedWriteSpinLock>(ScopedReadSpinLock& toLock, ScopedWriteSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseReadOnlyAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
            toLock.pLock->ConvertFromWriteToReadLock();
        }
    }

	template<>
	void TransferScopedLock<ScopedWriteSpinLock, ScopedWriteSpinLock>(ScopedWriteSpinLock& toLock, ScopedWriteSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseReadAndWriteAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
        }
    }

	template<>
	void TransferScopedLock<ScopedWriteSpinLock, ScopedReadSpinLock>(ScopedWriteSpinLock& toLock, ScopedReadSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseReadAndWriteAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
            toLock.pLock->ConvertFromReadToWriteLock();
        }
    }

    // Write-priority read-write lock transfer specializations
    template<>
	void TransferScopedLock<ScopedWritePriorityReadSpinLock, ScopedWritePriorityReadSpinLock>(ScopedWritePriorityReadSpinLock& toLock, ScopedWritePriorityReadSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseWritePriorityReadOnlyAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
        }
    }

	template<>
	void TransferScopedLock<ScopedWritePriorityReadSpinLock, ScopedWritePriorityWriteSpinLock>(ScopedWritePriorityReadSpinLock& toLock, ScopedWritePriorityWriteSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseWritePriorityReadOnlyAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
            toLock.pLock->ConvertFromWritePriorityWriteToReadLock();
        }
    }

	template<>
	void TransferScopedLock<ScopedWritePriorityWriteSpinLock, ScopedWritePriorityWriteSpinLock>(ScopedWritePriorityWriteSpinLock& toLock, ScopedWritePriorityWriteSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseWritePriorityReadAndWriteAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
        }
    }

	template<>
	void TransferScopedLock<ScopedWritePriorityWriteSpinLock, ScopedWritePriorityReadSpinLock>(ScopedWritePriorityWriteSpinLock& toLock, ScopedWritePriorityReadSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseWritePriorityReadAndWriteAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
            toLock.pLock->ConvertFromWritePriorityReadToWriteLock();
        }
    }

    // Multi-Reader/Writer lock transfer specializations
	template<>
	void TransferScopedLock<ScopedMultiReaderWriterReadSpinLock, ScopedMultiReaderWriterReadSpinLock>(ScopedMultiReaderWriterReadSpinLock& toLock, ScopedMultiReaderWriterReadSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseMultiReaderWriterReadAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
        }
    }

	template<>
	void TransferScopedLock<ScopedMultiReaderWriterReadSpinLock, ScopedMultiReaderWriterWriteSpinLock>(ScopedMultiReaderWriterReadSpinLock& toLock, ScopedMultiReaderWriterWriteSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseMultiReaderWriterReadAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
            toLock.pLock->ConvertFromMultiReaderWriterWriteToReadLock();
        }
    }


    template<>
	void TransferScopedLock<ScopedMultiReaderWriterWriteSpinLock, ScopedMultiReaderWriterWriteSpinLock>(ScopedMultiReaderWriterWriteSpinLock& toLock, ScopedMultiReaderWriterWriteSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseMultiReaderWriterWriteAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
        }
    }

	template<>
	void TransferScopedLock<ScopedMultiReaderWriterWriteSpinLock, ScopedMultiReaderWriterReadSpinLock>(ScopedMultiReaderWriterWriteSpinLock& toLock, ScopedMultiReaderWriterReadSpinLock&& fromLock)
    {
        if(toLock.pLock)
        {
            toLock.pLock->ReleaseMultiReaderWriterWriteAccess();
            toLock.pLock = nullptr;
        }

        if(fromLock.pLock)
        {
            toLock.pLock = fromLock.pLock;
            fromLock.pLock = nullptr;
            toLock.pLock->ConvertFromMultiReaderWriterReadToWriteLock();
        }
    }


}; //end namespace CoreTypes
}; //end namespace PklE

