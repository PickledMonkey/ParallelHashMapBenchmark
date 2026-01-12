#pragma once

#include <stdint.h>
#include <utility>
#include "atomic_util.h"

namespace PklE
{
namespace CoreTypes
{
    struct alignas(alignof(uint32_t)) CountingSpinlock
	{
		// Used in standard read-write lock implementations
		static inline constexpr uint32_t c_writeLockBit = 0x80000000;

		// Used in Multi-Reader/Writer lock implementations
		static inline constexpr uint32_t c_multiReaderWriter_WriteIncrement = 0x10000;
		static inline constexpr uint32_t c_multiReaderWriter_WriteMask = 0xFFFF0000;
		static inline constexpr uint32_t c_multiReaderWriter_ReadMask = 0x0000FFFF;

		uint32_t lockValue = 0;

        CountingSpinlock();
        CountingSpinlock(CountingSpinlock&& other);

        CountingSpinlock& operator=(CountingSpinlock&& other);

		// Standard read-write lock methods
		void AcquireReadOnlyAccess();
		void ReleaseReadOnlyAccess();
		void AcquireReadAndWriteAccess();
		void ReleaseReadAndWriteAccess();
		void ConvertFromReadToWriteLock();
		void ConvertFromWriteToReadLock();

		// Write priority read-write lock methods
		void AcquireWritePriorityReadOnlyAccess();
		void ReleaseWritePriorityReadOnlyAccess();
		void AcquireWritePriorityReadAndWriteAccess();
		void ReleaseWritePriorityReadAndWriteAccess();
		void ConvertFromWritePriorityReadToWriteLock();
		void ConvertFromWritePriorityWriteToReadLock();

		// Multi-Reader/Writer lock methods
		void AcquireMultiReaderWriterReadAccess();
		void ReleaseMultiReaderWriterReadAccess();
		void AcquireMultiReaderWriterWriteAccess();
		void ReleaseMultiReaderWriterWriteAccess();
		void ConvertFromMultiReaderWriterReadToWriteLock();
		void ConvertFromMultiReaderWriterWriteToReadLock();

	};

	template<typename ToLock_T, typename FromLock_T>
	void TransferScopedLock(ToLock_T& toLock, FromLock_T&& fromLock)
	{
		//Using sizeof() to generate a compile-time error if this function is called with an invalid type.
		static_assert(sizeof(ToLock_T) == 0, "TransferScopedLock is not implemented for the given types.");
	}

	struct ScopedReadSpinLock
	{
		CountingSpinlock* pLock = nullptr;

		ScopedReadSpinLock();
		ScopedReadSpinLock(CountingSpinlock& lock);
		ScopedReadSpinLock(CountingSpinlock* pLock);
		ScopedReadSpinLock(ScopedReadSpinLock&& other);
		
		template<typename FromLock_T>
		ScopedReadSpinLock(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
		}

		~ScopedReadSpinLock();

		template<typename FromLock_T>
		ScopedReadSpinLock& operator=(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
			return *this;
		}
	};

	struct ScopedWriteSpinLock
	{
		CountingSpinlock* pLock = nullptr;

		ScopedWriteSpinLock();
		ScopedWriteSpinLock(CountingSpinlock& lock);
		ScopedWriteSpinLock(CountingSpinlock* pLock);
		ScopedWriteSpinLock(ScopedWriteSpinLock&& other);
		
		template<typename FromLock_T>
		ScopedWriteSpinLock(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
		}

		~ScopedWriteSpinLock();
		ScopedWriteSpinLock& operator=(ScopedWriteSpinLock&& other);

		template<typename FromLock_T>
		ScopedWriteSpinLock& operator=(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
			return *this;
		}
	};

	struct ScopedWritePriorityReadSpinLock
	{
		CountingSpinlock* pLock = nullptr;

		ScopedWritePriorityReadSpinLock();
		ScopedWritePriorityReadSpinLock(CountingSpinlock& lock);
		ScopedWritePriorityReadSpinLock(CountingSpinlock* pLock);
		ScopedWritePriorityReadSpinLock(ScopedWritePriorityReadSpinLock&& other);

		template<typename FromLock_T>
		ScopedWritePriorityReadSpinLock(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
		}

		~ScopedWritePriorityReadSpinLock();

		template<typename FromLock_T>
		ScopedWritePriorityReadSpinLock& operator=(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
			return *this;
		}
	};

	struct ScopedWritePriorityWriteSpinLock
	{
		CountingSpinlock* pLock = nullptr;

		ScopedWritePriorityWriteSpinLock();
		ScopedWritePriorityWriteSpinLock(CountingSpinlock& lock);
		ScopedWritePriorityWriteSpinLock(CountingSpinlock* pLock);
		ScopedWritePriorityWriteSpinLock(ScopedWritePriorityWriteSpinLock&& other);

		template<typename FromLock_T>
		ScopedWritePriorityWriteSpinLock(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
		}

		~ScopedWritePriorityWriteSpinLock();
		ScopedWritePriorityWriteSpinLock& operator=(ScopedWritePriorityWriteSpinLock&& other);

		template<typename FromLock_T>
		ScopedWritePriorityWriteSpinLock& operator=(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
			return *this;
		}
	};

	struct ScopedMultiReaderWriterReadSpinLock
	{
		CountingSpinlock* pLock = nullptr;

		ScopedMultiReaderWriterReadSpinLock();
		ScopedMultiReaderWriterReadSpinLock(CountingSpinlock& lock);
		ScopedMultiReaderWriterReadSpinLock(CountingSpinlock* pLock);
		ScopedMultiReaderWriterReadSpinLock(ScopedMultiReaderWriterReadSpinLock&& other);
		~ScopedMultiReaderWriterReadSpinLock();
		ScopedMultiReaderWriterReadSpinLock& operator=(ScopedMultiReaderWriterReadSpinLock&& other);

		template<typename FromLock_T>
		ScopedMultiReaderWriterReadSpinLock(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
		}

		template<typename FromLock_T>
		ScopedMultiReaderWriterReadSpinLock& operator=(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
			return *this;
		}
	};

	struct ScopedMultiReaderWriterWriteSpinLock
	{
		CountingSpinlock* pLock = nullptr;

		ScopedMultiReaderWriterWriteSpinLock();
		ScopedMultiReaderWriterWriteSpinLock(CountingSpinlock& lock);
		ScopedMultiReaderWriterWriteSpinLock(CountingSpinlock* pLock);
		ScopedMultiReaderWriterWriteSpinLock(ScopedMultiReaderWriterWriteSpinLock&& other);
		~ScopedMultiReaderWriterWriteSpinLock();
		ScopedMultiReaderWriterWriteSpinLock& operator=(ScopedMultiReaderWriterWriteSpinLock&& other);

		template<typename FromLock_T>
		ScopedMultiReaderWriterWriteSpinLock(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
		}

		template<typename FromLock_T>
		ScopedMultiReaderWriterWriteSpinLock& operator=(FromLock_T&& fromLock)
		{
			TransferScopedLock(*this, std::forward<FromLock_T>(fromLock));
			return *this;
		}
	};

	// Standard read-write lock transfer specializations
	template<>
	void TransferScopedLock<ScopedReadSpinLock, ScopedReadSpinLock>(ScopedReadSpinLock& toLock, ScopedReadSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedReadSpinLock, ScopedWriteSpinLock>(ScopedReadSpinLock& toLock, ScopedWriteSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedWriteSpinLock, ScopedWriteSpinLock>(ScopedWriteSpinLock& toLock, ScopedWriteSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedWriteSpinLock, ScopedReadSpinLock>(ScopedWriteSpinLock& toLock, ScopedReadSpinLock&& fromLock);

	// Write-priority read-write lock transfer specializations
	template<>
	void TransferScopedLock<ScopedWritePriorityReadSpinLock, ScopedWritePriorityReadSpinLock>(ScopedWritePriorityReadSpinLock& toLock, ScopedWritePriorityReadSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedWritePriorityReadSpinLock, ScopedWritePriorityWriteSpinLock>(ScopedWritePriorityReadSpinLock& toLock, ScopedWritePriorityWriteSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedWritePriorityWriteSpinLock, ScopedWritePriorityWriteSpinLock>(ScopedWritePriorityWriteSpinLock& toLock, ScopedWritePriorityWriteSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedWritePriorityWriteSpinLock, ScopedWritePriorityReadSpinLock>(ScopedWritePriorityWriteSpinLock& toLock, ScopedWritePriorityReadSpinLock&& fromLock);

	// Multi-Reader/Writer lock transfer specializations
	template<>
	void TransferScopedLock<ScopedMultiReaderWriterReadSpinLock, ScopedMultiReaderWriterReadSpinLock>(ScopedMultiReaderWriterReadSpinLock& toLock, ScopedMultiReaderWriterReadSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedMultiReaderWriterReadSpinLock, ScopedMultiReaderWriterWriteSpinLock>(ScopedMultiReaderWriterReadSpinLock& toLock, ScopedMultiReaderWriterWriteSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedMultiReaderWriterWriteSpinLock, ScopedMultiReaderWriterWriteSpinLock>(ScopedMultiReaderWriterWriteSpinLock& toLock, ScopedMultiReaderWriterWriteSpinLock&& fromLock);
	template<>
	void TransferScopedLock<ScopedMultiReaderWriterWriteSpinLock, ScopedMultiReaderWriterReadSpinLock>(ScopedMultiReaderWriterWriteSpinLock& toLock, ScopedMultiReaderWriterReadSpinLock&& fromLock);



}; //end namespace CoreTypes
}; //end namespace PklE