#include "hashmap_benchmark.h"


// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunInsertTest(const KeyGenFunc& keyGen, const char* testLabelOverride = nullptr)
{
    HashmapType hashmap;
    auto setupFunc = [](auto& map) { map.clear(); };

    auto testLogic = CreateInsertOperation<KeyType, ValueType>(hashmap, keyGen, 16);

    std::string baseTestLabel = (testLabelOverride == nullptr) ? "insert" : testLabelOverride;
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }
    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunBatchInsertTest(const KeyGenFunc& keyGen)
{
    HashmapType hashmap;
    auto setupFunc = [](auto& map) { 
        map.clear(); 
        map.reserve(HashmapBenchmarkTest::OPERATIONS_PER_THREAD);
    };

    auto testLogic = CreateBatchedInsertOperation<KeyType, ValueType>(hashmap, keyGen, 16);

    std::string baseTestLabel = "batchInsert";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }
    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunLookupTest(const KeyGenFunc& keyGen)
{
    HashmapType hashmap;
    std::atomic<uint64_t> successCounter{0};

    auto setupFunc = [&keyGen](auto& map)
    {
        map.clear();
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::PRELOAD_KEYS, keyGen);
    };
    
    auto testLogic = CreateLookupOperation<KeyType, ValueType>(hashmap, keyGen, 16, successCounter);
    
    std::string baseTestLabel = "lookup";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }
    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
        
    ASSERT_GT(successCounter.load(), 0u);
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunBatchedLookupTest(const KeyGenFunc& keyGen)
{
    HashmapType hashmap;
    std::atomic<uint64_t> successCounter{0};

    auto setupFunc = [&keyGen](auto& map)
    {
        map.clear();
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::PRELOAD_KEYS, keyGen);
    };
    
    auto testLogic = CreateBatchedLookupOperation<KeyType, ValueType>(hashmap, keyGen, 16, successCounter);
    
    std::string baseTestLabel = "batchedLookup";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }
    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
        
    ASSERT_GT(successCounter.load(), 0u);
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunEraseTest(const KeyGenFunc& keyGen)
{
    HashmapType hashmap;
    std::atomic<uint64_t> successCounter{0};

    auto setupFunc = [&keyGen](auto& map)
    {
        map.clear();
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::PRELOAD_KEYS, keyGen);
    };
    
    auto testLogic = CreateEraseOperation<KeyType, ValueType>(hashmap, keyGen, 16, successCounter);
    
    std::string baseTestLabel = "erase";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }

    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
        
    ASSERT_GT(successCounter.load(), 0u);
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunMixedReadWriteTest(const KeyGenFunc& keyGen, const uint32_t readPercent, const uint32_t writePercent)
{
    HashmapType hashmap;
    std::atomic<uint64_t> readCounter{0};
    std::atomic<uint64_t> writeCounter{0};
    
    auto setupFunc = [&readCounter, &writeCounter, &keyGen](auto& map)
    {
        map.clear();
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::PRELOAD_KEYS, keyGen);
        readCounter = 0;
        writeCounter = 0;
    };

    auto testLogic = CreateMixedOperation<KeyType, ValueType>(hashmap, keyGen, 16, readCounter, writeCounter, 90);

    std::string baseTestLabel = std::to_string(readPercent) + "r" + std::to_string(writePercent) + "w";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if (sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }

    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
        
    ASSERT_GT(readCounter.load(), 0u);
    ASSERT_GT(writeCounter.load(), 0u);
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunMixedWithEraseTest(const KeyGenFunc& keyGen, uint32_t insertPercent, uint32_t lookupPercent, uint32_t erasePercent)
{
    HashmapType hashmap;
    std::atomic<uint64_t> insertCounter{0};
    std::atomic<uint64_t> lookupCounter{0};
    std::atomic<uint64_t> eraseCounter{0};

    auto setupFunc = [&insertCounter, &lookupCounter, &eraseCounter, &keyGen](auto& map)
    {
        map.clear();
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::PRELOAD_KEYS, keyGen);
        insertCounter = 0;
        lookupCounter = 0;
        eraseCounter = 0;
    };

    auto testLogic = CreateComplexMixedOperation<KeyType, ValueType>(
        hashmap, keyGen, 16,
        insertCounter, lookupCounter, eraseCounter,
        insertPercent, lookupPercent, erasePercent);

    std::string baseTestLabel = std::to_string(insertPercent) + "i" +
                            std::to_string(lookupPercent) + "l" +
                            std::to_string(erasePercent) + "e";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if (sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }

    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
        
    ASSERT_GT(insertCounter.load(), 0u);
    ASSERT_GT(lookupCounter.load(), 0u);
    ASSERT_GT(eraseCounter.load(), 0u);
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunRekeyTest(const KeyGenFunc& keyGen)
{
    constexpr uint64_t KEY_OFFSET = 10000000ULL;

    HashmapType hashmap;
    std::atomic<uint64_t> successCounter{0};
    
    // Preload the map with sequential keys
    auto setupFunc = [keyGen, &successCounter](auto& map)
    {
        map.clear();
        successCounter.store(0);
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::OPERATIONS_PER_THREAD, keyGen);
    };

    auto testLogic = CreateRekeyOperation<KeyType, ValueType>(hashmap, keyGen, 16, successCounter, KEY_OFFSET);

    std::string baseTestLabel = "rekey";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }
    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::OPERATIONS_PER_THREAD,
        baseTestLabel.c_str());
        
    ASSERT_GT(successCounter.load(), 0u);
}

template<typename KeyType, typename ValueType, typename HashmapType, typename KeyGenFunc>
void RunIteratorTest(const KeyGenFunc& keyGen)
{
    HashmapType hashmap;
    std::atomic<uint64_t> iterationCounter{0};
    
    // Preload the map with sequential keys
    auto setupFunc = [keyGen, &iterationCounter](auto& map)
    {
        map.clear();
        iterationCounter.store(0);
        HashmapBenchmarkTest::PreloadHashmap(map, HashmapBenchmarkTest::OPERATIONS_PER_THREAD, keyGen);
    };

    auto testLogic = CreateIteratorOperation<KeyType, ValueType>(hashmap, iterationCounter);

    std::string baseTestLabel = "iterator";
    std::string testLabel = baseTestLabel;

    std::string keyGenName = KeyGenerator::GetKeyGenName(keyGen);
    testLabel += keyGenName;

    if(sizeof(ValueType) > sizeof(uint64_t))
    {
        testLabel += "BigValue";
    }
    std::string labeledTestName = std::string(HashmapType::GetMapTypeName()) + "_" + testLabel;

    const bool bSingleThreadedOnly = true;
    HashmapBenchmarkTest::RunThreadScalingBenchmark(
        labeledTestName.c_str(),
        hashmap,
        setupFunc,
        testLogic,
        HashmapBenchmarkTest::ITERATOR_OPERATIONS,
        baseTestLabel.c_str(),
        bSingleThreadedOnly);
        
    ASSERT_GT(iterationCounter.load(), 0u);
}


// ============================================================================
// STD::UNORDERED_MAP LOCKED WRAPPER
// ============================================================================
TEST_F(HashmapInsertTest, StdUnorderedMapLocked_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, StdUnorderedMapLocked_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, StdUnorderedMapLocked_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, StdUnorderedMapLocked_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, StdUnorderedMapLocked_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, StdUnorderedMapLocked_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, StdUnorderedMapLocked_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, StdUnorderedMapLocked_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, StdUnorderedMapLocked_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, StdUnorderedMapLocked_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, StdUnorderedMapLocked_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, StdUnorderedMapLocked_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, StdUnorderedMapLocked_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, StdUnorderedMapLocked_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, StdUnorderedMapLocked_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, StdUnorderedMapLocked_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, StdUnorderedMapLocked_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, StdUnorderedMapLocked_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, StdUnorderedMapLocked_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        90,
        10);
}

TEST_F(HashmapMixedTest, StdUnorderedMapLocked_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        90,
        10);
}

TEST_F(HashmapMixedTest, StdUnorderedMapLocked_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        50,
        50);
}

TEST_F(HashmapMixedTest, StdUnorderedMapLocked_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        50,
        50);
}

TEST_F(HashmapMixedTest, StdUnorderedMapLocked_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        40,
        50,
        10);
}

TEST_F(HashmapMixedTest, StdUnorderedMapLocked_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        40,
        50,
        10);
}

TEST_F(HashmapContendedTest, StdUnorderedMapLocked_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, StdUnorderedMapLocked_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, StdUnorderedMapLocked_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, StdUnorderedMapLocked_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, StdUnorderedMapLocked_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, StdUnorderedMapLocked_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, StdUnorderedMapLocked_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, StdUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, StdUnorderedMapLocked_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, StdUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}



// ============================================================================
// PKLE HASHMAP TESTS LOCKLESS - PklE::ThreadsafeContainers::HashMap without using internal locking
// ============================================================================
TEST_F(HashmapInsertTest, PklEHashMapLockless_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PklEHashMapLockless_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PklEHashMapLockless_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, PklEHashMapLockless_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PklEHashMapLockless_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PklEHashMapLockless_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PklEHashMapLockless_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PklEHashMapLockless_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PklEHashMapLockless_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PklEHashMapLockless_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PklEHashMapLockless_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PklEHashMapLockless_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMapLockless_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMapLockless_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMapLockless_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMapLockless_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, PklEHashMapLockless_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, PklEHashMapLockless_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, PklEHashMapLockless_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PklEHashMapLockless_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PklEHashMapLockless_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PklEHashMapLockless_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PklEHashMapLockless_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, PklEHashMapLockless_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, PklEHashMapLockless_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, PklEHashMapLockless_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, PklEHashMapLockless_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, PklEHashMapLockless_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PklEHashMapLockless_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PklEHashMapLockless_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PklEHashMapLockless_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, true>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, PklEHashMapLockless_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, true>>(KeyGenerator::Random);
}

// ============================================================================
// PKLE HASHMAP TESTS CONCURRENT - PklE::ThreadsafeContainers::HashMap using internal locking
// ============================================================================
TEST_F(HashmapInsertTest, PklEHashMap_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PklEHashMap_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PklEHashMap_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, PklEHashMap_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PklEHashMap_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PklEHashMap_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PklEHashMap_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PklEHashMap_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PklEHashMap_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PklEHashMap_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PklEHashMap_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PklEHashMap_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMap_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMap_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMap_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PklEHashMap_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, PklEHashMap_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, PklEHashMap_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}


TEST_F(HashmapMixedTest, PklEHashMap_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PklEHashMap_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PklEHashMap_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PklEHashMap_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PklEHashMap_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, PklEHashMap_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, PklEHashMap_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, PklEHashMap_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, PklEHashMap_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, PklEHashMap_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PklEHashMap_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PklEHashMap_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PklEHashMap_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, PklEHashMap<uint64_t, uint64_t, false>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, PklEHashMap_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PklEHashMap<uint64_t, TestValueStruct, false>>(KeyGenerator::Random);
}

// ============================================================================
// PHMAP SPINLOCK TESTS - Parallel Flat Hash Map with Spinlock (Standard R/W Lock)
// ============================================================================

TEST_F(HashmapInsertTest, PhmapSpinlock_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PhmapSpinlock_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PhmapSpinlock_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, PhmapSpinlock_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PhmapSpinlock_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PhmapSpinlock_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PhmapSpinlock_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PhmapSpinlock_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PhmapSpinlock_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PhmapSpinlock_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PhmapSpinlock_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PhmapSpinlock_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PhmapSpinlock_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PhmapSpinlock_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PhmapSpinlock_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PhmapSpinlock_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, PhmapSpinlock_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, PhmapSpinlock_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, PhmapSpinlock_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PhmapSpinlock_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PhmapSpinlock_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PhmapSpinlock_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PhmapSpinlock_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, PhmapSpinlock_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, PhmapSpinlock_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, PhmapSpinlock_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, PhmapSpinlock_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, PhmapSpinlock_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapSpinlock_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapSpinlock_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapSpinlock_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, PhmapParallelFlatHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, PhmapSpinlock_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PhmapParallelFlatHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}


#if PKLE_INCLUDE_ABSEIL_HASHMAP
// ============================================================================
// ABSEIL TESTS - Flat Hash Map with External Locking
// ============================================================================

TEST_F(HashmapInsertTest, AbseilFlatHashMapLocked_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, AbseilFlatHashMapLocked_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, AbseilFlatHashMapLocked_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, AbseilFlatHashMapLocked_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, AbseilFlatHashMapLocked_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, AbseilFlatHashMapLocked_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, AbseilFlatHashMapLocked_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, AbseilFlatHashMapLocked_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, AbseilFlatHashMapLocked_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, AbseilFlatHashMapLocked_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, AbseilFlatHashMapLocked_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, AbseilFlatHashMapLocked_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, AbseilFlatHashMapLocked_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, AbseilFlatHashMapLocked_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, AbseilFlatHashMapLocked_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, AbseilFlatHashMapLocked_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, AbseilFlatHashMapLocked_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, AbseilFlatHashMapLocked_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, AbseilFlatHashMapLocked_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, AbseilFlatHashMapLocked_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, AbseilFlatHashMapLocked_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, AbseilFlatHashMapLocked_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, AbseilFlatHashMapLocked_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, AbseilFlatHashMapLocked_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, AbseilFlatHashMapLocked_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, AbseilFlatHashMapLocked_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, AbseilFlatHashMapLocked_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, AbseilFlatHashMapLocked_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilFlatHashMapLocked_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilFlatHashMapLocked_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilFlatHashMapLocked_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, AbseilFlatHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, AbseilFlatHashMapLocked_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, AbseilFlatHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

// ============================================================================
// ABSEIL NODE HASH MAP TESTS - Node Hash Map with External Locking
// ============================================================================

TEST_F(HashmapInsertTest, AbseilNodeHashMapLocked_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, AbseilNodeHashMapLocked_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, AbseilNodeHashMapLocked_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, AbseilNodeHashMapLocked_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapLocked_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapLocked_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapLocked_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapLocked_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapLocked_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapLocked_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapLocked_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapLocked_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapLocked_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapLocked_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapLocked_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapLocked_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, AbseilNodeHashMapLocked_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, AbseilNodeHashMapLocked_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapLocked_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        90,
        10);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapLocked_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        90,
        10);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapLocked_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        50,
        50);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapLocked_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        50,
        50);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapLocked_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        40,
        50,
        10);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapLocked_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        40,
        50,
        10);
}

TEST_F(HashmapContendedTest, AbseilNodeHashMapLocked_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, AbseilNodeHashMapLocked_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, AbseilNodeHashMapLocked_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, AbseilNodeHashMapLocked_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapLocked_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapLocked_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapLocked_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, AbseilNodeHashMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapLocked_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, AbseilNodeHashMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

// ============================================================================
// ABSEIL NODE HASH MAP WITH PAGING ALLOCATOR TESTS
// ============================================================================

TEST_F(HashmapInsertTest, AbseilNodeHashMapPagingAllocator_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, AbseilNodeHashMapPagingAllocator_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, AbseilNodeHashMapPagingAllocator_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, AbseilNodeHashMapPagingAllocator_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapPagingAllocator_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapPagingAllocator_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapPagingAllocator_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, AbseilNodeHashMapPagingAllocator_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapPagingAllocator_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapPagingAllocator_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapPagingAllocator_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, AbseilNodeHashMapPagingAllocator_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapPagingAllocator_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapPagingAllocator_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapPagingAllocator_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, AbseilNodeHashMapPagingAllocator_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, AbseilNodeHashMapPagingAllocator_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, AbseilNodeHashMapPagingAllocator_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapPagingAllocator_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        90,
        10);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapPagingAllocator_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        90,
        10);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapPagingAllocator_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        50,
        50);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapPagingAllocator_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        50,
        50);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapPagingAllocator_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(
        KeyGenerator::Sequential,
        40,
        50,
        10);
}

TEST_F(HashmapMixedTest, AbseilNodeHashMapPagingAllocator_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(
        KeyGenerator::Sequential,
        40,
        50,
        10);
}

TEST_F(HashmapContendedTest, AbseilNodeHashMapPagingAllocator_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, AbseilNodeHashMapPagingAllocator_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, AbseilNodeHashMapPagingAllocator_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, AbseilNodeHashMapPagingAllocator_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapPagingAllocator_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapPagingAllocator_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapPagingAllocator_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, AbseilNodeHashMapPagingAllocator<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, AbseilNodeHashMapPagingAllocator_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, AbseilNodeHashMapPagingAllocator<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}
#endif //PKLE_INCLUDE_ABSEIL_HASHMAP

#if PKLE_INCLUDE_PARLAY_HASHMAP
// ============================================================================
// PARLAY HASH TESTS - Parlay Unordered Map with External Locking
// ============================================================================

TEST_F(HashmapInsertTest, ParlayUnorderedMapLocked_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, ParlayUnorderedMapLocked_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, ParlayUnorderedMapLocked_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, ParlayUnorderedMapLocked_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, ParlayUnorderedMapLocked_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, ParlayUnorderedMapLocked_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, ParlayUnorderedMapLocked_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, ParlayUnorderedMapLocked_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, ParlayUnorderedMapLocked_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, ParlayUnorderedMapLocked_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, ParlayUnorderedMapLocked_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, ParlayUnorderedMapLocked_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, ParlayUnorderedMapLocked_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, ParlayUnorderedMapLocked_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, ParlayUnorderedMapLocked_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, ParlayUnorderedMapLocked_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, ParlayUnorderedMapLocked_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, ParlayUnorderedMapLocked_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, ParlayUnorderedMapLocked_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, ParlayUnorderedMapLocked_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, ParlayUnorderedMapLocked_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, ParlayUnorderedMapLocked_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, ParlayUnorderedMapLocked_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, ParlayUnorderedMapLocked_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, ParlayUnorderedMapLocked_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, ParlayUnorderedMapLocked_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, ParlayUnorderedMapLocked_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, ParlayUnorderedMapLocked_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, ParlayUnorderedMapLocked_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, ParlayUnorderedMapLocked_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, ParlayUnorderedMapLocked_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, ParlayUnorderedMapLocked<uint64_t, uint64_t>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, ParlayUnorderedMapLocked_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, ParlayUnorderedMapLocked<uint64_t, TestValueStruct>>(KeyGenerator::Random);
}
#endif //PKLE_INCLUDE_PARLAY_HASHMAP

// ============================================================================
// PHMAP NODE HASH MAP SPINLOCK TESTS
// ============================================================================

TEST_F(HashmapInsertTest, PhmapNodeHashMapSpinlock_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PhmapNodeHashMapSpinlock_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PhmapNodeHashMapSpinlock_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, PhmapNodeHashMapSpinlock_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapSpinlock_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapSpinlock_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapSpinlock_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapSpinlock_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapSpinlock_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapSpinlock_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapSpinlock_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapSpinlock_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapSpinlock_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapSpinlock_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapSpinlock_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapSpinlock_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, PhmapNodeHashMapSpinlock_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, PhmapNodeHashMapSpinlock_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapSpinlock_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapSpinlock_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapSpinlock_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapSpinlock_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapSpinlock_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapSpinlock_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, PhmapNodeHashMapSpinlock_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, PhmapNodeHashMapSpinlock_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, PhmapNodeHashMapSpinlock_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, PhmapNodeHashMapSpinlock_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapSpinlock_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapSpinlock_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapSpinlock_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, PhmapParallelNodeHashMapSpinlock<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapSpinlock_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapSpinlock<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}


// ============================================================================
// PHMAP NODE HASH MAP WITH PAGING ALLOCATOR TESTS
// ============================================================================

TEST_F(HashmapInsertTest, PhmapNodeHashMapPagingAllocator_InsertSequential)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PhmapNodeHashMapPagingAllocator_InsertSequentialBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapInsertTest, PhmapNodeHashMapPagingAllocator_InsertRandom)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapInsertTest, PhmapNodeHashMapPagingAllocator_InsertRandomBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapPagingAllocator_BatchInsertSequential)
{
    RunBatchInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapPagingAllocator_BatchInsertSequentialBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapPagingAllocator_BatchInsertRandom)
{
    RunBatchInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchInsertTest, PhmapNodeHashMapPagingAllocator_BatchInsertRandomBigValue)
{
    RunBatchInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapPagingAllocator_LookupSequential)
{
    RunLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapPagingAllocator_LookupSequentialBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapPagingAllocator_LookupRandom)
{
    RunLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapLookupTest, PhmapNodeHashMapPagingAllocator_LookupRandomBigValue)
{
    RunLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapPagingAllocator_BatchedLookupSequential)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapPagingAllocator_BatchedLookupSequentialBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapPagingAllocator_BatchedLookupRandom)
{
    RunBatchedLookupTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapBatchedLookupTest, PhmapNodeHashMapPagingAllocator_BatchedLookupRandomBigValue)
{
    RunBatchedLookupTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapEraseTest, PhmapNodeHashMapPagingAllocator_EraseSequential)
{
    RunEraseTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapEraseTest, PhmapNodeHashMapPagingAllocator_EraseSequentialBigValue)
{
    RunEraseTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapPagingAllocator_90r10w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapPagingAllocator_90r10wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 90, 10);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapPagingAllocator_50r50w)
{
    RunMixedReadWriteTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapPagingAllocator_50r50wBigValue)
{
    RunMixedReadWriteTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 50, 50);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapPagingAllocator_40i50l10e)
{
    RunMixedWithEraseTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapMixedTest, PhmapNodeHashMapPagingAllocator_40i50l10eBigValue)
{
    RunMixedWithEraseTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential, 40, 50, 10);
}

TEST_F(HashmapContendedTest, PhmapNodeHashMapPagingAllocator_ContendedInsert)
{
    RunInsertTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapContendedTest, PhmapNodeHashMapPagingAllocator_ContendedInsertBigValue)
{
    RunInsertTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Contended, "contendedInsert");
}

TEST_F(HashmapRekeyTest, PhmapNodeHashMapPagingAllocator_RekeySequential)
{
    RunRekeyTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapRekeyTest, PhmapNodeHashMapPagingAllocator_RekeySequentialBigValue)
{
    RunRekeyTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapPagingAllocator_IteratorsSequential)
{
    RunIteratorTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapPagingAllocator_IteratorsSequentialBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Sequential);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapPagingAllocator_IteratorsRandom)
{
    RunIteratorTest<uint64_t, uint64_t, PhmapParallelNodeHashMapPagingAllocator<uint64_t, uint64_t, 4>>(KeyGenerator::Random);
}

TEST_F(HashmapIteratorTest, PhmapNodeHashMapPagingAllocator_IteratorsRandomBigValue)
{
    RunIteratorTest<uint64_t, TestValueStruct, PhmapParallelNodeHashMapPagingAllocator<uint64_t, TestValueStruct, 4>>(KeyGenerator::Random);
}
