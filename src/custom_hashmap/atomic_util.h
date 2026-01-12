#pragma once

#include <stdint.h>
#include <stddef.h>

#define ATOMIC_UTIL_BUILD_TOOLSET_GCC 1
#define ATOMIC_UTIL_BUILD_TOOLSET_CLANG 2
#define ATOMIC_UTIL_BUILD_TOOLSET_MSVC 3

#define ATOMIC_UTIL_BUILD_TOOLSET ATOMIC_UTIL_BUILD_TOOLSET_GCC

#if (ATOMIC_UTIL_BUILD_TOOLSET == ATOMIC_UTIL_BUILD_TOOLSET_MSVC)
#define PKLE_ATOMIC_MEMORDER_IGNORE_MEMORY_BARRIERS 0
#define PKLE_ATOMIC_MEMORDER_ENFORCE_MEMORY_BARRIERS 1
#endif

namespace PklE
{
namespace Util
{
    enum class MemoryOrder : int32_t
    {
        #if (ATOMIC_UTIL_BUILD_TOOLSET != ATOMIC_UTIL_BUILD_TOOLSET_MSVC)
        RELAXED = __ATOMIC_RELAXED,
        CONSUME = __ATOMIC_CONSUME,
        ACQUIRE = __ATOMIC_ACQUIRE,
        RELEASE = __ATOMIC_RELEASE,
        ACQ_REL = __ATOMIC_ACQ_REL,
        SEQ_CST = __ATOMIC_SEQ_CST,
        #else
        RELAXED = PKLE_ATOMIC_MEMORDER_IGNORE_MEMORY_BARRIERS,
        CONSUME = PKLE_ATOMIC_MEMORDER_ENFORCE_MEMORY_BARRIERS,
        ACQUIRE = PKLE_ATOMIC_MEMORDER_ENFORCE_MEMORY_BARRIERS,
        RELEASE = PKLE_ATOMIC_MEMORDER_ENFORCE_MEMORY_BARRIERS,
        ACQ_REL = PKLE_ATOMIC_MEMORDER_ENFORCE_MEMORY_BARRIERS,
        SEQ_CST = PKLE_ATOMIC_MEMORDER_ENFORCE_MEMORY_BARRIERS,
        #endif //ATOMIC_UTIL_BUILD_TOOLSET!= ATOMIC_UTIL_BUILD_TOOLSET_MSVC

        INVALID
    };

    //Atomic Increment and decrement functions. Operations are applied and the result is returned.
    uint8_t AtomicIncrementU8(uint8_t& value);
    uint8_t AtomicDecrementU8(uint8_t& value);

    int8_t AtomicIncrementI8(int8_t& value);
    int8_t AtomicDecrementI8(int8_t& value);

    uint16_t AtomicIncrementU16(uint16_t& value);
    uint16_t AtomicDecrementU16(uint16_t& value);

    int16_t AtomicIncrementI16(int16_t& value);
    int16_t AtomicDecrementI16(int16_t& value);

    uint32_t AtomicIncrementU32(uint32_t& value);
    uint32_t AtomicDecrementU32(uint32_t& value);

    int32_t AtomicIncrementI32(int32_t& value);
    int32_t AtomicDecrementI32(int32_t& value);

    uint64_t AtomicIncrementU64(uint64_t& value);
    uint64_t AtomicDecrementU64(uint64_t& value);

    int64_t AtomicIncrementI64(int64_t& value);
    int64_t AtomicDecrementI64(int64_t& value);

    //Atomic Add and Subtract functions. Operations are applied and the result is returned.
    uint8_t AtomicAddU8(uint8_t& value, uint8_t addend);
    uint8_t AtomicSubtractU8(uint8_t& value, uint8_t subtrahend);

    int8_t AtomicAddI8(int8_t& value, int8_t addend);
    int8_t AtomicSubtractI8(int8_t& value, int8_t subtrahend);

    uint16_t AtomicAddU16(uint16_t& value, uint16_t addend);
    uint16_t AtomicSubtractU16(uint16_t& value, uint16_t subtrahend);

    int16_t AtomicAddI16(int16_t& value, int16_t addend);
    int16_t AtomicSubtractI16(int16_t& value, int16_t subtrahend);

    uint32_t AtomicAddU32(uint32_t& value, uint32_t addend);
    uint32_t AtomicSubtractU32(uint32_t& value, uint32_t subtrahend);

    int32_t AtomicAddI32(int32_t& value, int32_t addend);
    int32_t AtomicSubtractI32(int32_t& value, int32_t subtrahend);

    uint64_t AtomicAddU64(uint64_t& value, uint64_t addend);
    uint64_t AtomicSubtractU64(uint64_t& value, uint64_t subtrahend);

    int64_t AtomicAddI64(int64_t& value, int64_t addend);
    int64_t AtomicSubtractI64(int64_t& value, int64_t subtrahend);

    //Atomic Exchange functions. The original value is returned.
    uint8_t AtomicExchangeU8(uint8_t& value, uint8_t newValue);
    int8_t AtomicExchangeI8(int8_t& value, int8_t newValue);

    uint16_t AtomicExchangeU16(uint16_t& value, uint16_t newValue);
    int16_t AtomicExchangeI16(int16_t& value, int16_t newValue);

    uint32_t AtomicExchangeU32(uint32_t& value, uint32_t newValue);
    int32_t AtomicExchangeI32(int32_t& value, int32_t newValue);

    uint64_t AtomicExchangeU64(uint64_t& value, uint64_t newValue);
    int64_t AtomicExchangeI64(int64_t& value, int64_t newValue);

    //Atomic Compare and Exchange functions. Returns true if successful. May fail spuriously, so use the strong versions if that is a concern.
    bool AtomicCompareExchangeU8(uint8_t& value, uint8_t newValue, uint8_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeI8(int8_t& value, int8_t newValue, int8_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeBool(bool& value, bool newValue, bool comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangeU16(uint16_t& value, uint16_t newValue, uint16_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeI16(int16_t& value, int16_t newValue, int16_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangeU32(uint32_t& value, uint32_t newValue, uint32_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    inline bool AtomicCompareExchangeU32(_Atomic uint32_t& value, uint32_t newValue, uint32_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED) 
    {
        return AtomicCompareExchangeU32(*(reinterpret_cast<uint32_t*>(&value)), newValue, comparand, successOrder, failureOrder); 
    }

    bool AtomicCompareExchangeI32(int32_t& value, int32_t newValue, int32_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangeU64(uint64_t& value, uint64_t newValue, uint64_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    inline bool AtomicCompareExchangeU64(_Atomic uint64_t& value, uint64_t newValue, uint64_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED) 
    { 
        return AtomicCompareExchangeU64(*(reinterpret_cast<uint64_t*>(&value)), newValue, comparand, successOrder, failureOrder); 
    }

    bool AtomicCompareExchangeI64(int64_t& value, int64_t newValue, int64_t comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangePtr(void** value, void* newValue, void* comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    template<typename T>
    bool AtomicCompareExchangePtrT(T*& value, T* newValue, T* comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED)
    {
        return AtomicCompareExchangePtr(reinterpret_cast<void**>(&value), reinterpret_cast<void*>(newValue), reinterpret_cast<void*>(comparand), successOrder, failureOrder);
    }

    template<typename T>
    bool AtomicCompareExchangePtrT(_Atomic (T*)& value, T* newValue, T* comparand, MemoryOrder successOrder = MemoryOrder::RELAXED, MemoryOrder failureOrder = MemoryOrder::RELAXED)
    {
        return AtomicCompareExchangePtr(reinterpret_cast<void**>(&value), reinterpret_cast<void*>(newValue), reinterpret_cast<void*>(comparand), successOrder, failureOrder);
    }

    //Strong Compare and Exchange functions. Returns true if the exchange was successful.
    bool AtomicCompareExchangeStrongU8(uint8_t& value, uint8_t newValue, uint8_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeStrongI8(int8_t& value, int8_t newValue, int8_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeStrongBool(bool& value, bool newValue, bool comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangeStrongU16(uint16_t& value, uint16_t newValue, uint16_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeStrongI16(int16_t& value, int16_t newValue, int16_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangeStrongU32(uint32_t& value, uint32_t newValue, uint32_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeStrongI32(int32_t& value, int32_t newValue, int32_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    bool AtomicCompareExchangeStrongU64(uint64_t& value, uint64_t newValue, uint64_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeStrongI64(int64_t& value, int64_t newValue, int64_t comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);
    bool AtomicCompareExchangeStrongPtr(void** value, void* newValue, void* comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED);

    template<typename T>
    bool AtomicCompareExchangeStrongPtrT(T*& value, T* newValue, T* comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED)
    {
        return AtomicCompareExchangeStrongPtr(reinterpret_cast<void**>(&value), reinterpret_cast<void*>(newValue), reinterpret_cast<void*>(comparand), successOrder, failureOrder);
    }

    template<typename T>
    bool AtomicCompareExchangeStrongPtrT(_Atomic (T*)& value, T* newValue, T* comparand, MemoryOrder successOrder = MemoryOrder::RELEASE, MemoryOrder failureOrder = MemoryOrder::RELAXED)
    {
        return AtomicCompareExchangeStrongPtr(reinterpret_cast<void**>(&value), reinterpret_cast<void*>(newValue), reinterpret_cast<void*>(comparand), successOrder, failureOrder);
    }

    //Atomic Bitwise functions. The original value is returned.
    uint8_t AtomicAndU8(uint8_t& value, uint8_t mask);
    uint8_t AtomicOrU8(uint8_t& value, uint8_t mask);
    uint8_t AtomicXorU8(uint8_t& value, uint8_t mask);

    uint16_t AtomicAndU16(uint16_t& value, uint16_t mask);
    uint16_t AtomicOrU16(uint16_t& value, uint16_t mask);
    uint16_t AtomicXorU16(uint16_t& value, uint16_t mask);

    uint32_t AtomicAndU32(uint32_t& value, uint32_t mask);
    uint32_t AtomicOrU32(uint32_t& value, uint32_t mask);
    uint32_t AtomicXorU32(uint32_t& value, uint32_t mask);

    uint64_t AtomicAndU64(uint64_t& value, uint64_t mask);
    uint64_t AtomicOrU64(uint64_t& value, uint64_t mask);
    uint64_t AtomicXorU64(uint64_t& value, uint64_t mask);


    //Atomic loads and stores
    uint8_t AtomicLoadU8(const uint8_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreU8(uint8_t& value, uint8_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    uint8_t AtomicLoadI8(const int8_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreI8(int8_t& value, int8_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    uint16_t AtomicLoadU16(const uint16_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreU16(uint16_t& value, uint16_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    uint16_t AtomicLoadI16(const int16_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreI16(int16_t& value, int16_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    uint32_t AtomicLoadU32(const uint32_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreU32(uint32_t& value, uint32_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    inline void AtomicStoreU32(_Atomic uint32_t& value, uint32_t newValue, MemoryOrder order = MemoryOrder::RELAXED)
    {
        AtomicStoreU32(*(reinterpret_cast<uint32_t*>(&value)), newValue, order);
    }

    uint32_t AtomicLoadI32(const int32_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreI32(int32_t& value, int32_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    uint64_t AtomicLoadU64(const uint64_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreU64(uint64_t& value, uint64_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    uint64_t AtomicLoadI64(const int64_t& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStoreI64(int64_t& value, int64_t newValue, MemoryOrder order = MemoryOrder::RELAXED);

    void* AtomicLoadPtr(void* const& value, MemoryOrder order = MemoryOrder::RELAXED);
    void AtomicStorePtr(void** value, void* newValue, MemoryOrder order = MemoryOrder::RELAXED);

    // Fences
    void AtomicThreadFence(MemoryOrder order);
    void AtomicSignalFence(MemoryOrder order);

    template<typename T>
    T* AtomicLoadPtrT(T* const& value, MemoryOrder order = MemoryOrder::RELAXED)
    {
        return reinterpret_cast<T*>(AtomicLoadPtr(reinterpret_cast<void* const&>(value), order));
    }
    template<typename T>
    void AtomicStorePtrT(T*& value, T* newValue, MemoryOrder order = MemoryOrder::RELAXED)
    {
        AtomicStorePtr(reinterpret_cast<void**>(&value), reinterpret_cast<void*>(newValue), order);
    }

    template<typename T>
    T AtomicIncrement(T& value)
    {
        static_assert(sizeof(T) == -1, "AtomicIncrement not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicIncrement<uint8_t>(uint8_t& value) {return AtomicIncrementU8(value);}

    template<>
    inline int8_t AtomicIncrement<int8_t>(int8_t& value) {return AtomicIncrementI8(value);}

    template<>
    inline uint16_t AtomicIncrement<uint16_t>(uint16_t& value) {return AtomicIncrementU16(value);}

    template<>
    inline int16_t AtomicIncrement<int16_t>(int16_t& value) {return AtomicIncrementI16(value);}

    template<>
    inline uint32_t AtomicIncrement<uint32_t>(uint32_t& value) {return AtomicIncrementU32(value);}

    template<>
    inline int32_t AtomicIncrement<int32_t>(int32_t& value) {return AtomicIncrementI32(value);}

    template<>
    inline uint64_t AtomicIncrement<uint64_t>(uint64_t& value) {return AtomicIncrementU64(value);}

    template<>
    inline int64_t AtomicIncrement<int64_t>(int64_t& value) {return AtomicIncrementI64(value);}

    template<typename T>
    T AtomicDecrement(T& value)
    {
        static_assert(sizeof(T) == -1, "AtomicDecrement not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicDecrement<uint8_t>(uint8_t& value) {return AtomicDecrementU8(value);}

    template<>
    inline int8_t AtomicDecrement<int8_t>(int8_t& value) {return AtomicDecrementI8(value);}

    template<>
    inline uint16_t AtomicDecrement<uint16_t>(uint16_t& value) {return AtomicDecrementU16(value);}

    template<>
    inline int16_t AtomicDecrement<int16_t>(int16_t& value) {return AtomicDecrementI16(value);}

    template<>
    inline uint32_t AtomicDecrement<uint32_t>(uint32_t& value) {return AtomicDecrementU32(value);}

    template<>
    inline int32_t AtomicDecrement<int32_t>(int32_t& value) {return AtomicDecrementI32(value);}

    template<>
    inline uint64_t AtomicDecrement<uint64_t>(uint64_t& value) {return AtomicDecrementU64(value);}

    template<>
    inline int64_t AtomicDecrement<int64_t>(int64_t& value) {return AtomicDecrementI64(value);}

    template<typename T>
    T AtomicAdd(T& value, T addend)
    {
        static_assert(sizeof(T) == -1, "AtomicAdd not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicAdd<uint8_t>(uint8_t& value, uint8_t addend) {return AtomicAddU8(value, addend);}

    template<>
    inline int8_t AtomicAdd<int8_t>(int8_t& value, int8_t addend) {return AtomicAddI8(value, addend);}

    template<>
    inline uint16_t AtomicAdd<uint16_t>(uint16_t& value, uint16_t addend) {return AtomicAddU16(value, addend);}

    template<>
    inline int16_t AtomicAdd<int16_t>(int16_t& value, int16_t addend) {return AtomicAddI16(value, addend);}

    template<>
    inline uint32_t AtomicAdd<uint32_t>(uint32_t& value, uint32_t addend) {return AtomicAddU32(value, addend);}

    template<>
    inline int32_t AtomicAdd<int32_t>(int32_t& value, int32_t addend) {return AtomicAddI32(value, addend);}

    template<>
    inline uint64_t AtomicAdd<uint64_t>(uint64_t& value, uint64_t addend) {return AtomicAddU64(value, addend);}

    template<>
    inline int64_t AtomicAdd<int64_t>(int64_t& value, int64_t addend) {return AtomicAddI64(value, addend);}

    template<typename T>
    T AtomicSubtract(T& value, T subtrahend)
    {
        static_assert(sizeof(T) == -1, "AtomicSubtract not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicSubtract<uint8_t>(uint8_t& value, uint8_t subtrahend) {return AtomicSubtractU8(value, subtrahend);}

    template<>
    inline int8_t AtomicSubtract<int8_t>(int8_t& value, int8_t subtrahend) {return AtomicSubtractI8(value, subtrahend);}

    template<>
    inline uint16_t AtomicSubtract<uint16_t>(uint16_t& value, uint16_t subtrahend) {return AtomicSubtractU16(value, subtrahend);}

    template<>
    inline int16_t AtomicSubtract<int16_t>(int16_t& value, int16_t subtrahend) {return AtomicSubtractI16(value, subtrahend);}

    template<>
    inline uint32_t AtomicSubtract<uint32_t>(uint32_t& value, uint32_t subtrahend) {return AtomicSubtractU32(value, subtrahend);}

    template<>
    inline int32_t AtomicSubtract<int32_t>(int32_t& value, int32_t subtrahend) {return AtomicSubtractI32(value, subtrahend);}

    template<>
    inline uint64_t AtomicSubtract<uint64_t>(uint64_t& value, uint64_t subtrahend) {return AtomicSubtractU64(value, subtrahend);}

    template<>
    inline int64_t AtomicSubtract<int64_t>(int64_t& value, int64_t subtrahend) {return AtomicSubtractI64(value, subtrahend);}

    template<typename T>
    T AtomicExchange(T& value, T newValue)
    {
        static_assert(sizeof(T) == -1, "AtomicExchange not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicExchange<uint8_t>(uint8_t& value, uint8_t newValue) {return AtomicExchangeU8(value, newValue);}

    template<>
    inline int8_t AtomicExchange<int8_t>(int8_t& value, int8_t newValue) {return AtomicExchangeI8(value, newValue);}

    template<>
    inline uint16_t AtomicExchange<uint16_t>(uint16_t& value, uint16_t newValue) {return AtomicExchangeU16(value, newValue);}

    template<>
    inline int16_t AtomicExchange<int16_t>(int16_t& value, int16_t newValue) {return AtomicExchangeI16(value, newValue);}

    template<>
    inline uint32_t AtomicExchange<uint32_t>(uint32_t& value, uint32_t newValue) {return AtomicExchangeU32(value, newValue);}

    template<>
    inline int32_t AtomicExchange<int32_t>(int32_t& value, int32_t newValue) {return AtomicExchangeI32(value, newValue);}

    template<>
    inline uint64_t AtomicExchange<uint64_t>(uint64_t& value, uint64_t newValue) {return AtomicExchangeU64(value, newValue);}

    template<>
    inline int64_t AtomicExchange<int64_t>(int64_t& value, int64_t newValue) {return AtomicExchangeI64(value, newValue);}

    template<typename T>
    bool AtomicCompareExchange(T& value, T newValue, T comparand, const MemoryOrder successOrder = MemoryOrder::RELAXED, const MemoryOrder failureOrder = MemoryOrder::RELAXED)
    {
        static_assert(sizeof(T) == -1, "AtomicCompareExchange not implemented for this type.");
        return false;
    }

    template<>
    inline bool AtomicCompareExchange<uint8_t>(uint8_t& value, uint8_t newValue, uint8_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeU8(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<int8_t>(int8_t& value, int8_t newValue, int8_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeI8(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<bool>(bool& value, bool newValue, bool comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeBool(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<uint16_t>(uint16_t& value, uint16_t newValue, uint16_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeU16(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<int16_t>(int16_t& value, int16_t newValue, int16_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeI16(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<uint32_t>(uint32_t& value, uint32_t newValue, uint32_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeU32(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<int32_t>(int32_t& value, int32_t newValue, int32_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeI32(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<uint64_t>(uint64_t& value, uint64_t newValue, uint64_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeU64(value, newValue, comparand, successOrder, failureOrder);}

    template<>
    inline bool AtomicCompareExchange<int64_t>(int64_t& value, int64_t newValue, int64_t comparand, const MemoryOrder successOrder, const MemoryOrder failureOrder) {return AtomicCompareExchangeI64(value, newValue, comparand, successOrder, failureOrder);}

    template<typename T>
    bool AtomicCompareExchangeStrong(T& value, T newValue, T comparand)
    {
        static_assert(sizeof(T) == -1, "AtomicCompareExchangeStrong not implemented for this type.");
        return false;
    }

    template<>
    inline bool AtomicCompareExchangeStrong<uint8_t>(uint8_t& value, uint8_t newValue, uint8_t comparand) {return AtomicCompareExchangeStrongU8(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<int8_t>(int8_t& value, int8_t newValue, int8_t comparand) {return AtomicCompareExchangeStrongI8(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<bool>(bool& value, bool newValue, bool comparand) {return AtomicCompareExchangeStrongBool(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<uint16_t>(uint16_t& value, uint16_t newValue, uint16_t comparand) {return AtomicCompareExchangeStrongU16(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<int16_t>(int16_t& value, int16_t newValue, int16_t comparand) {return AtomicCompareExchangeStrongI16(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<uint32_t>(uint32_t& value, uint32_t newValue, uint32_t comparand) {return AtomicCompareExchangeStrongU32(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<int32_t>(int32_t& value, int32_t newValue, int32_t comparand) {return AtomicCompareExchangeStrongI32(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<uint64_t>(uint64_t& value, uint64_t newValue, uint64_t comparand) {return AtomicCompareExchangeStrongU64(value, newValue, comparand);}

    template<>
    inline bool AtomicCompareExchangeStrong<int64_t>(int64_t& value, int64_t newValue, int64_t comparand) {return AtomicCompareExchangeStrongI64(value, newValue, comparand);}

    template<typename T>
    T AtomicAnd(T& value, T mask)
    {
        static_assert(sizeof(T) == -1, "AtomicAnd not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicAnd<uint8_t>(uint8_t& value, uint8_t mask) {return AtomicAndU8(value, mask);}

    template<>
    inline uint16_t AtomicAnd<uint16_t>(uint16_t& value, uint16_t mask) {return AtomicAndU16(value, mask);}

    template<>
    inline uint32_t AtomicAnd<uint32_t>(uint32_t& value, uint32_t mask) {return AtomicAndU32(value, mask);}

    template<>
    inline uint64_t AtomicAnd<uint64_t>(uint64_t& value, uint64_t mask) {return AtomicAndU64(value, mask);}

    template<typename T>
    T AtomicOr(T& value, T mask)
    {
        static_assert(sizeof(T) == -1, "AtomicOr not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicOr<uint8_t>(uint8_t& value, uint8_t mask) {return AtomicOrU8(value, mask);}

    template<>
    inline uint16_t AtomicOr<uint16_t>(uint16_t& value, uint16_t mask) {return AtomicOrU16(value, mask);}

    template<>
    inline uint32_t AtomicOr<uint32_t>(uint32_t& value, uint32_t mask) {return AtomicOrU32(value, mask);}

    template<>
    inline uint64_t AtomicOr<uint64_t>(uint64_t& value, uint64_t mask) {return AtomicOrU64(value, mask);}

    template<typename T>
    T AtomicXor(T& value, T mask)
    {
        static_assert(sizeof(T) == -1, "AtomicXor not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicXor<uint8_t>(uint8_t& value, uint8_t mask) {return AtomicXorU8(value, mask);}

    template<>
    inline uint16_t AtomicXor<uint16_t>(uint16_t& value, uint16_t mask) {return AtomicXorU16(value, mask);}

    template<>
    inline uint32_t AtomicXor<uint32_t>(uint32_t& value, uint32_t mask) {return AtomicXorU32(value, mask);}

    template<>
    inline uint64_t AtomicXor<uint64_t>(uint64_t& value, uint64_t mask) {return AtomicXorU64(value, mask);}


    template<typename T>
    T AtomicLoad(const T& value, MemoryOrder order = MemoryOrder::RELAXED)
    {
        static_assert(sizeof(T) == -1, "AtomicLoad not implemented for this type.");
        return value;
    }

    template<>
    inline uint8_t AtomicLoad<uint8_t>(const uint8_t& value, MemoryOrder order) {return AtomicLoadU8(value, order);}
    template<>
    inline int8_t AtomicLoad<int8_t>(const int8_t& value, MemoryOrder order) {return AtomicLoadI8(value, order);}
    template<>
    inline uint16_t AtomicLoad<uint16_t>(const uint16_t& value, MemoryOrder order) {return AtomicLoadU16(value, order);}
    template<>
    inline int16_t AtomicLoad<int16_t>(const int16_t& value, MemoryOrder order) {return AtomicLoadI16(value, order);}
    template<>
    inline uint32_t AtomicLoad<uint32_t>(const uint32_t& value, MemoryOrder order) {return AtomicLoadU32(value, order);}
    template<>
    inline int32_t AtomicLoad<int32_t>(const int32_t& value, MemoryOrder order) {return AtomicLoadI32(value, order);}
    template<>
    inline uint64_t AtomicLoad<uint64_t>(const uint64_t& value, MemoryOrder order) {return AtomicLoadU64(value, order);}
    template<>
    inline int64_t AtomicLoad<int64_t>(const int64_t& value, MemoryOrder order) {return AtomicLoadI64(value, order);}
    template<>
    inline void* AtomicLoad<void*>(void* const& value, MemoryOrder order) {return AtomicLoadPtr(value, order);}

    template<typename T>
    void AtomicStore(T& value, T newValue, MemoryOrder order = MemoryOrder::RELAXED)
    {
        static_assert(sizeof(T) == -1, "AtomicStore not implemented for this type.");
    }
    template<>
    inline void AtomicStore<uint8_t>(uint8_t& value, uint8_t newValue, MemoryOrder order) {AtomicStoreU8(value, newValue, order);}
    template<>
    inline void AtomicStore<int8_t>(int8_t& value, int8_t newValue, MemoryOrder order) {AtomicStoreI8(value, newValue, order);}
    template<>
    inline void AtomicStore<uint16_t>(uint16_t& value, uint16_t newValue, MemoryOrder order) {AtomicStoreU16(value, newValue, order);}
    template<>
    inline void AtomicStore<int16_t>(int16_t& value, int16_t newValue, MemoryOrder order) {AtomicStoreI16(value, newValue, order);}
    template<>
    inline void AtomicStore<uint32_t>(uint32_t& value, uint32_t newValue, MemoryOrder order) {AtomicStoreU32(value, newValue, order);}
    template<>
    inline void AtomicStore<int32_t>(int32_t& value, int32_t newValue, MemoryOrder order) {AtomicStoreI32(value, newValue, order);}
    template<>
    inline void AtomicStore<uint64_t>(uint64_t& value, uint64_t newValue, MemoryOrder order) {AtomicStoreU64(value, newValue, order);}
    template<>
    inline void AtomicStore<int64_t>(int64_t& value, int64_t newValue, MemoryOrder order) {AtomicStoreI64(value, newValue, order);}
    template<>
    inline void AtomicStore<void*>(void*& value, void* newValue, MemoryOrder order) {AtomicStorePtr(reinterpret_cast<void**>(&value), newValue, order);}


    template<const size_t Alignment_T>
    struct AtomicAlignasHelper
    {
        inline static constexpr size_t Alignment = Alignment_T;
    };

    template<>
    struct AtomicAlignasHelper<1>
    {
        inline static constexpr size_t Alignment = 2; //Use 2 byte alignment for 1 byte types to avoid potential false sharing issues on some platforms
    };



}; //end namespace Util
}; //end namespace PklE

#define PKLE_DECLARE_ATOMIC_ALIGNED(type, name) alignas(PklE::Util::AtomicAlignasHelper<sizeof(type)>::Alignment) type name