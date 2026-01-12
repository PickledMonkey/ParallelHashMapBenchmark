#pragma once

#include <stdint.h>

namespace PklE
{
namespace Util
{
    // Static const table of prime numbers for hash table sizing
    inline constexpr uint32_t c_powerOfTwoPrimeNumbers[] = {
        1,          3,          7,          13,         31,
        53,         89,         211,        431,        827,
        1663,       4093,       8191,       16381,      32749,
        65519,      131071,     262139,     524287,     1048573,
        1572869,    2097143,    4194287,    8388587,    16777213,
        33554383,   67108859,   134217593,  268435367,  536870909,
        1073741789, 2147483647
    };
    inline constexpr uint32_t c_numPowerOfTwoPrimes = static_cast<uint32_t>(sizeof(c_powerOfTwoPrimeNumbers) / sizeof(c_powerOfTwoPrimeNumbers[0]));

    inline uint32_t GetNextPowerOfTwoTablePrime(uint32_t currentSize)
    {
        for(uint32_t i = 0; i < c_numPowerOfTwoPrimes; ++i)
        {
            if(c_powerOfTwoPrimeNumbers[i] >= currentSize)
            {
                return c_powerOfTwoPrimeNumbers[i];
            }
        }
        //If no larger prime found, return the largest prime
        return c_powerOfTwoPrimeNumbers[c_numPowerOfTwoPrimes - 1];
    }

    inline constexpr uint32_t c_powerOfTwoTable[] = {
        1, 2, 4, 8, 16, 32, 64, 128,
        256, 512, 1024, 2048, 4096, 8192,
        16384, 32768, 65536, 131072, 262144,
        524288, 1048576, 2097152, 4194304,
        8388608, 16777216, 33554432, 67108864,
        134217728, 268435456, 536870912,
        1073741824, 2147483648u
    };
    inline constexpr uint32_t c_numPowerOfTwoEntries = static_cast<uint32_t>(sizeof(c_powerOfTwoTable) / sizeof(c_powerOfTwoTable[0]));
    inline uint32_t GetNextPowerOfTwo(uint32_t value)
    {
        for(uint32_t i = 0; i < c_numPowerOfTwoEntries; ++i)
        {
            if(c_powerOfTwoTable[i] >= value)
            {
                return c_powerOfTwoTable[i];
            }
        }
        //If no larger power of two found, return the largest power of two
        return c_powerOfTwoTable[c_numPowerOfTwoEntries - 1];
    }

    template<uint32_t Value_T>
    inline constexpr uint32_t GetNextPowerOfTwoConstexpr()
    {
        for(uint32_t i = 0; i < c_numPowerOfTwoEntries; ++i)
        {
            if(c_powerOfTwoTable[i] >= Value_T)
            {
                return c_powerOfTwoTable[i];
            }
        }
        //If no larger power of two found, return the largest power of two
        return c_powerOfTwoTable[c_numPowerOfTwoEntries - 1];
    }

    inline static constexpr uint64_t c_fibonacciConstant = 11400714819323198485llu; //based on: 2^64 / golden ratio constant.
    inline uint32_t FibonnaciHash(uint64_t hash, const uint64_t shiftAmount)
    {
        // Fibonacci Hashing to map hash to bucket index
        // Code Taken from: https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
        hash ^= hash >> shiftAmount;
        uint32_t index = static_cast<uint32_t>((c_fibonacciConstant * hash) >> shiftAmount);
        return index;
    }

}; //end namespace Util
}; //end namespace PklE