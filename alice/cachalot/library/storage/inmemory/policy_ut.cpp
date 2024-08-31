#include <alice/cachalot/library/storage/inmemory/imdb.h>
#include <alice/cachalot/library/storage/inmemory/lfu_policy.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/guid.h>
#include <util/random/random.h>


struct TKey {
    TString String;
    TGUID Guid;

    TKey() = default;

    explicit TKey(TString string)
        : String(std::move(string))
    {
        CreateGuid(&Guid);
    }

    uint64_t GetMemoryUsage() const {
        return sizeof(*this) + String.capacity();
    }

    bool operator==(const TKey& other) const {
        return Guid == other.Guid && String == other.String;
    }
};

template<>
struct THash<TKey> {
    uint64_t operator()(const TKey& key) const {
        return CombineHashes(THash<TGUID>{}(key.Guid), THash<TString>{}(key.String));
    }
};

struct TData {
    int Id = 0;
    TString Name;
    TString Surname;

    TData() = default;

    TData(int id, TString name, TString surname)
        : Id(id)
        , Name(std::move(name))
        , Surname(std::move(surname))
    {
    }

    uint64_t GetMemoryUsage() const {
        return sizeof(*this) + Name.capacity() + Surname.capacity();
    }

    bool operator==(const TData& other) const {
        return Id == other.Id && Name == other.Name && Surname == other.Surname;
    }
};

struct TFixedSizeObj {
    uint32_t Id = 0;
    uint64_t MemoryUsage = 0;

    TFixedSizeObj() = default;

    explicit TFixedSizeObj(uint32_t id, uint64_t mu)
        : Id(id)
        , MemoryUsage(mu)
    {
    }

    uint64_t GetMemoryUsage() const {
        return MemoryUsage;
    }

    bool operator==(const TFixedSizeObj& other) const {
        return Id == other.Id;
    }
};

template<>
struct THash<TFixedSizeObj> {
    uint64_t operator()(const TFixedSizeObj& obj) const {
        return obj.Id;
    }
};

TString GenString(size_t len) {
    static const TString letters = "PriFfEt@MIDved<3";
    TString result(len, '\0');
    for (char& x : result) {
        x = letters[RandomNumber<unsigned>() % 16];
    }
    return result;
}


using TFifoPolicyType = NCachalot::NPrivate::TFifoPolicy<TKey>;
using TFifoCacheType = NCachalot::TFifoCache<TKey, TData>;

using TLfuPolicyType = NCachalot::NPrivate::TLfuPolicy<TKey>;
using TLfuCacheType = NCachalot::TLfuCache<TKey, TData>;

using TLruPolicyType = NCachalot::NPrivate::TLruPolicy<TKey>;
using TLruCacheType = NCachalot::TLruCache<TKey, TData>;

using TReadOncePolicyType = NCachalot::NPrivate::TReadOncePolicy<TKey>;
using TReadOnceCacheType = NCachalot::TReadOnceCache<TKey, TData>;


Y_UNIT_TEST_SUITE(PolicyTest) {


Y_UNIT_TEST(Fifo) {
    TFifoPolicyType policy;

    const TKey key1(GenString(20));
    const TKey key2(GenString(30));
    const TKey key3(GenString(40));

    UNIT_ASSERT_UNEQUAL(key1, key2);
    UNIT_ASSERT_UNEQUAL(key1, key3);
    UNIT_ASSERT_UNEQUAL(key2, key3);

    policy.OnNewItem(key1);  // [1]
    policy.OnNewItem(key2);  // [1, 2]
    policy.OnTouch(key2);  // [1, 2]

    auto res1 = policy.GetKeyToRemove();  // [2]
    UNIT_ASSERT(res1.Defined());
    UNIT_ASSERT_EQUAL(res1.GetRef(), key1);

    policy.OnNewItem(key3);  // [2, 3]
    policy.OnTouch(key3);  // [2, 3]
    policy.OnTouch(key3);  // [2, 3]

    auto res2 = policy.GetKeyToRemove();  // [3]
    UNIT_ASSERT(res2.Defined());
    UNIT_ASSERT_EQUAL(res2.GetRef(), key2);

    policy.OnNewItem(key1);  // [3, 1]

    auto res3 = policy.GetKeyToRemove();  // [1]
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), key3);
}

Y_UNIT_TEST(Lfu) {
    TLfuPolicyType policy;

    const TKey key1(GenString(20));
    const TKey key2(GenString(30));
    const TKey key3(GenString(40));

    UNIT_ASSERT_UNEQUAL(key1, key2);
    UNIT_ASSERT_UNEQUAL(key1, key3);
    UNIT_ASSERT_UNEQUAL(key2, key3);

    policy.OnNewItem(key1);  // {1: [1]}
    policy.OnNewItem(key2);  // {1: [1, 2]}
    policy.OnTouch(key2);  // {1: [1], 2: [2]}

    auto res1 = policy.GetKeyToRemove();
    UNIT_ASSERT(res1.Defined());
    UNIT_ASSERT_EQUAL(res1.GetRef(), key1);

    policy.OnNewItem(key3);  // {1: [3], 2: [2]}
    policy.OnTouch(key3);  // {2: [2, 3]}
    policy.OnTouch(key3);  // {2: [2], 3: [3]}

    auto res2 = policy.GetKeyToRemove();
    UNIT_ASSERT(res2.Defined());
    UNIT_ASSERT_EQUAL(res2.GetRef(), key2);

    policy.OnNewItem(key1);  // {1: [1], 3: [3]}

    auto res3 = policy.GetKeyToRemove();
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), key1);
}

Y_UNIT_TEST(Lru) {
    TLruPolicyType policy;

    const TKey key1(GenString(20));
    const TKey key2(GenString(30));
    const TKey key3(GenString(40));

    UNIT_ASSERT_UNEQUAL(key1, key2);
    UNIT_ASSERT_UNEQUAL(key1, key3);
    UNIT_ASSERT_UNEQUAL(key2, key3);

    policy.OnNewItem(key1);  // [1]
    policy.OnNewItem(key2);  // [2, 1]
    policy.OnTouch(key2);  // [2, 1]

    auto res1 = policy.GetKeyToRemove();  // [2]
    UNIT_ASSERT(res1.Defined());
    UNIT_ASSERT_EQUAL(res1.GetRef(), key1);

    policy.OnNewItem(key3);  // [3, 2]
    policy.OnTouch(key3);  // [3, 2]
    policy.OnTouch(key3);  // [3, 2]

    auto res2 = policy.GetKeyToRemove();  // [3]
    UNIT_ASSERT(res2.Defined());
    UNIT_ASSERT_EQUAL(res2.GetRef(), key2);

    policy.OnNewItem(key1);  // [1, 3]

    auto res3 = policy.GetKeyToRemove();  // [1]
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), key3);
}

Y_UNIT_TEST(ReadOnce) {
    TReadOncePolicyType policy;

    const TKey key1(GenString(20));
    const TKey key2(GenString(30));
    const TKey key3(GenString(40));

    UNIT_ASSERT_UNEQUAL(key1, key2);
    UNIT_ASSERT_UNEQUAL(key1, key3);
    UNIT_ASSERT_UNEQUAL(key2, key3);

    policy.OnNewItem(key1);  // [1]
    policy.OnNewItem(key2);  // [2, 1]
    policy.OnTouch(key2);  // [1, 2]

    auto res1 = policy.GetKeyToRemove();  // [1]
    UNIT_ASSERT(res1.Defined());
    UNIT_ASSERT_EQUAL(res1.GetRef(), key2);

    policy.OnNewItem(key3);  // [3, 1]

    auto res2 = policy.GetKeyToRemove();  // [3]
    UNIT_ASSERT(res2.Defined());
    UNIT_ASSERT_EQUAL(res2.GetRef(), key1);

    policy.OnNewItem(key1);  // [1, 3]
    policy.OnTouch(key3);  // [1, 3]

    auto res3 = policy.GetKeyToRemove();  // [1]
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), key3);

    policy.OnTouch(key1);  // [1]

    auto res4 = policy.GetKeyToRemove();  // []
    UNIT_ASSERT(res4.Defined());
    UNIT_ASSERT_EQUAL(res4.GetRef(), key1);

    auto res5 = policy.GetKeyToRemove();  // []
    UNIT_ASSERT(!res5.Defined());
}

};


Y_UNIT_TEST_SUITE(LfuCacheTest) {

Y_UNIT_TEST(SetGet) {
    const size_t maxElements = 1000;     // unlimited
    const size_t maxMemUsage = 1 << 30;  // unlimited

    TLfuCacheType map(maxElements, maxMemUsage);

    const TKey key1(GenString(20));
    const TData data1(123, GenString(123), GenString(123));
    const TKey key2(GenString(30));
    const TData data2(456, GenString(456), GenString(456));
    const TKey key3(GenString(40));

    UNIT_ASSERT_UNEQUAL(key1, key2);
    UNIT_ASSERT_UNEQUAL(key1, key3);
    UNIT_ASSERT_UNEQUAL(key2, key3);
    UNIT_ASSERT_UNEQUAL(data1, data2);


    map.Store(key1, data1);
    auto res1 = map.Load(key1);
    UNIT_ASSERT(res1.Defined());
    UNIT_ASSERT_EQUAL(res1.GetRef(), data1);

    map.Store(key2, data2);
    auto res2 = map.Load(key2);
    UNIT_ASSERT(res2.Defined());
    UNIT_ASSERT_EQUAL(res2.GetRef(), data2);

    auto res3 = map.Load(key3);
    UNIT_ASSERT(!res3.Defined());

    map.Store(key2, data1);
    auto res4 = map.Load(key2);
    UNIT_ASSERT(res4.Defined());
    UNIT_ASSERT_EQUAL(res4.GetRef(), data1);

    UNIT_ASSERT_LE(map.GetCurrentSize(), maxElements);
    UNIT_ASSERT_LE(map.GetCurrentMemoryUsage(), maxMemUsage);
}

Y_UNIT_TEST(LimitByNumberOfElements) {
    const size_t maxElements = 2;
    const size_t maxMemUsage = 1 << 30;  // unlimited

    TLfuCacheType map(maxElements, maxMemUsage);

    const TKey key1(GenString(10));
    const TKey key2(GenString(20));
    const TKey key3(GenString(30));
    const TData data1(1, GenString(10), GenString(10));
    const TData data2(2, GenString(20), GenString(20));
    const TData data3(3, GenString(30), GenString(30));

    map.Store(key1, data1);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 1);

    map.Store(key2, data2);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 2);

    // touch key1
    map.Load(key1);

    map.Store(key3, data3);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 2);

    auto res3 = map.Load(key3);
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), data3);

    // key2 removed
    auto res2 = map.Load(key2);
    UNIT_ASSERT(!res2.Defined());

    // key1 saved
    auto res1 = map.Load(key1);
    UNIT_ASSERT(res1.Defined());

    UNIT_ASSERT_LE(map.GetCurrentSize(), maxElements);
    UNIT_ASSERT_LE(map.GetCurrentMemoryUsage(), maxMemUsage);
}

Y_UNIT_TEST(LimitByMemoryUsage) {
    const size_t maxElements = 1000; // unlimited
    const size_t itemSize = 10000;
    const size_t maxMemUsage = 3 * itemSize;  // only 2 items fit in memory because of overheads

    using TKey = TFixedSizeObj;
    using TData = TFixedSizeObj;

    NCachalot::TLfuCache<TKey, TData> map(maxElements, maxMemUsage);

    const TKey key1(1, 0);
    const TKey key2(2, 0);
    const TKey key3(3, 0);
    const TData data1(1, itemSize);
    const TData data2(2, itemSize);
    const TData data3(3, itemSize);

    map.Store(key1, data1);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 1);

    map.Store(key2, data2);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 2);

    // touch key1
    map.Load(key1);

    map.Store(key3, data3);
    UNIT_ASSERT_LE(map.GetCurrentSize(), 2);
    UNIT_ASSERT_LE(1, map.GetCurrentSize());

    auto res3 = map.Load(key3);
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), data3);

    // key2 removed
    auto res2 = map.Load(key2);
    UNIT_ASSERT(!res2.Defined());

    UNIT_ASSERT_LE(map.GetCurrentSize(), maxElements);
    UNIT_ASSERT_LE(map.GetCurrentMemoryUsage(), maxMemUsage);
}

};


Y_UNIT_TEST_SUITE(FifoCacheTest) {

Y_UNIT_TEST(LimitByNumberOfElements) {
    const size_t maxElements = 2;
    const size_t maxMemUsage = 1 << 30;  // unlimited

    TFifoCacheType map(maxElements, maxMemUsage);

    const TKey key1(GenString(10));
    const TKey key2(GenString(20));
    const TKey key3(GenString(30));
    const TData data1(1, GenString(10), GenString(10));
    const TData data2(2, GenString(20), GenString(20));
    const TData data3(3, GenString(30), GenString(30));

    map.Store(key1, data1);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 1);

    map.Store(key2, data2);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 2);

    // touch key1
    map.Load(key1);

    map.Store(key3, data3);
    UNIT_ASSERT_VALUES_EQUAL(map.GetCurrentSize(), 2);

    auto res3 = map.Load(key3);
    UNIT_ASSERT(res3.Defined());
    UNIT_ASSERT_EQUAL(res3.GetRef(), data3);

    // key1 removed
    auto res2 = map.Load(key1);
    UNIT_ASSERT(!res2.Defined());

    // key2 saved
    auto res1 = map.Load(key2);
    UNIT_ASSERT(res1.Defined());

    UNIT_ASSERT_LE(map.GetCurrentSize(), maxElements);
    UNIT_ASSERT_LE(map.GetCurrentMemoryUsage(), maxMemUsage);
}

};
