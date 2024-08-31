#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/megamind_session/storage.h>
#include <alice/cachalot/library/storage/inmemory/imdb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/random/random.h>

using namespace NCachalot;
using namespace NCachalot::NPrivate;

float Perc(uint64_t a, uint64_t b) {
    return (a * 100.0) / b;
}

struct TItemWithSize {
    TItemWithSize() = default;

    explicit TItemWithSize(uint64_t size)
        : Size(size)
    {
    }

    uint64_t GetMemoryUsage() const {
        return Size;
    }

    bool operator==(const TItemWithSize& other) const {
        return Size == other.Size;
    }

    uint64_t Size;
};

template<>
struct THash<TItemWithSize> {
    size_t operator()(const TItemWithSize& value) const {
        return 0x1d23f4253726c9acull * value.Size;
    }
};


Y_UNIT_TEST_SUITE(InmemoryStorageTest) {

Y_UNIT_TEST(ExpiratioOrderTestWithConstantTtl) {
    static TInstant CurrentTime = TInstant::Now();

    struct TClock {
        static TInstant GetActualTimestamp() {
            SleepMcs(1);
            return CurrentTime;
        }

        static void SleepMcs(int mcs) {
            CurrentTime += TDuration::MicroSeconds(mcs);
        }
    };

    using TKey = TItemWithSize;

    const TDuration ttl = TDuration::MilliSeconds(20);
    TExpirationOrder<TKey, TClock> order;
    constexpr size_t n = 1273;
    uint64_t totalSize = 0;

    for (size_t i = 0; i < n; i += 2) {
        const uint64_t size = i + 1;
        totalSize += size;
        order.Push(TKey(size), ttl);
    }

    TClock::SleepMcs(20'000);

    for (size_t i = 1; i < n; i += 2) {
        const uint64_t size = i + 1;
        totalSize += size;
        order.Push(TKey(size), ttl);
    }

    TClock::SleepMcs(5'000);

    {
        auto expirationHelper = order.GetRemovalHelper();
        size_t count = 0;
        for (const TKey& key : expirationHelper.ListExpired()) {
            UNIT_ASSERT_VALUES_EQUAL(key.Size % 2, 1);
            ++count;
            totalSize -= key.Size;
        }
        expirationHelper.EraseExpired();
        UNIT_ASSERT_VALUES_EQUAL(count, (n + 1) / 2);
    }

    {
        auto expirationHelper = order.GetRemovalHelper();
        for (const TKey& key : expirationHelper.ListExpired()) {
            Y_UNUSED(key);
            UNIT_ASSERT(false);
        }
        expirationHelper.EraseExpired();
    }

    TClock::SleepMcs(20'000);

    {
        auto expirationHelper = order.GetRemovalHelper();
        size_t count = 0;
        for (const TKey& key : expirationHelper.ListExpired()) {
            UNIT_ASSERT_VALUES_EQUAL(key.Size % 2, 0);
            ++count;
        }
        expirationHelper.EraseExpired();
        UNIT_ASSERT_VALUES_EQUAL(count, n / 2);
    }
};

Y_UNIT_TEST(ExpiratioOrderTestWithCustomTtlAndTouch) {
    static TInstant CurrentTime = TInstant::Now();

    struct TClock {
        static TInstant GetActualTimestamp() {
            SleepMcs(1);
            return CurrentTime;
        }

        static void SleepMcs(int mcs) {
            CurrentTime += TDuration::MicroSeconds(mcs);
        }
    };

    using TKey = TItemWithSize;

    TExpirationOrder<TKey, TClock> order;

    order.Push(TKey(1), TDuration::MicroSeconds(1000));
    order.Push(TKey(2), TDuration::MicroSeconds(2000));
    order.Push(TKey(3), TDuration::MicroSeconds(3000));

    order.Touch(TKey(1));
    order.Touch(TKey(1));

    TClock::SleepMcs(1600);

    {
        auto expirationHelper = order.GetRemovalHelper();
        size_t count = 0;
        for (const TKey& key : expirationHelper.ListExpired()) {
            UNIT_ASSERT_VALUES_EQUAL(key.Size, 1);
            ++count;
        }
        expirationHelper.EraseExpired();
        UNIT_ASSERT_VALUES_EQUAL(count, 1);
    }

    order.Touch(TKey(2));

    TClock::SleepMcs(1600);

    {
        auto expirationHelper = order.GetRemovalHelper();
        size_t count = 0;
        for (const TKey& key : expirationHelper.ListExpired()) {
            UNIT_ASSERT_VALUES_EQUAL(key.Size, 3);
            ++count;
        }
        expirationHelper.EraseExpired();
        UNIT_ASSERT_VALUES_EQUAL(count, 1);
    }

    TClock::SleepMcs(400);

    {
        auto expirationHelper = order.GetRemovalHelper();
        size_t count = 0;
        for (const TKey& key : expirationHelper.ListExpired()) {
            UNIT_ASSERT_VALUES_EQUAL(key.Size, 2);
            ++count;
        }
        expirationHelper.EraseExpired();
        UNIT_ASSERT_VALUES_EQUAL(count, 1);
    }
};


Y_UNIT_TEST(AssociativeCacheWithConstantTtl) {
    static TInstant CurrentTime = TInstant::Now();

    struct TClock {
        static TInstant GetActualTimestamp() {
            SleepMcs(1);
            return CurrentTime;
        }

        static void SleepMcs(int mcs) {
            CurrentTime += TDuration::MicroSeconds(mcs);
        }
    };


    using TKey = TItemWithSize;
    using TData = TItemWithSize;
    using TCache = TLfuCache<TKey, TData, TClock>;

    constexpr uint64_t maxElements = 10;
    constexpr uint64_t maxMemoryUsage = 10000;
    const TDuration ttl = TDuration::MilliSeconds(15);

    TCache cache(
        maxElements + 1,
        maxMemoryUsage,
        ttl,
        true
    );

    for (uint64_t i = 0; i < maxElements; ++i) {
        cache.Store(TKey(i), TData(i));
        TClock::SleepMcs(1000);
    }

    for (uint64_t i = 0; i < maxElements; ++i) {
        const TMaybe<TKey> rsp = cache.Load(TKey(i));
        UNIT_ASSERT(rsp.Defined());
        UNIT_ASSERT_VALUES_EQUAL(rsp.GetRef().Size, i);
        TClock::SleepMcs(1000);
    }

    // Check that all items were Touched by Load.
    for (uint64_t i = 0; i < maxElements; ++i) {
        const TMaybe<TKey> rsp = cache.Load(TKey(i));
        UNIT_ASSERT(rsp.Defined());
        UNIT_ASSERT_VALUES_EQUAL(rsp.GetRef().Size, i);
        TClock::SleepMcs(1000);
    }

    TClock::SleepMcs(5500);

    for (uint64_t i = 0; i < maxElements; ++i) {
        const TMaybe<TKey> rsp = cache.Load(TKey(maxElements - 1 - i));
        if (5 <= maxElements - 1 - i) {
            // Items 5..9 are touched (live time extended, usage increased)
            UNIT_ASSERT(rsp.Defined());
            TClock::SleepMcs(1000);
        } else {
            // Items 0..4 are removed by ttl
            UNIT_ASSERT(!rsp.Defined());
        }
    }

    // Adding 10 new items with keys 10..19.
    // During the process item 9 will expire.
    // Since items 5..8 have a lot of usages, they will stay in cache.
    // Thus random 4 new items must be removed because they have equally small usage.
    for (uint64_t i = maxElements; i < 2 * maxElements; ++i) {
        cache.Store(TKey(i), TData(0));
        TClock::SleepMcs(1000);
    }

    size_t newItemsCounter = 0;
    for (uint64_t i = 0; i < 2 * maxElements; ++i) {
        const TMaybe<TKey> rsp = cache.Load(TKey(i));
        if (10 <= i) {
            if (rsp.Defined()) {
                ++newItemsCounter;
            }
        } else if (9 == i) {
            UNIT_ASSERT(!rsp.Defined());
        } else if (5 <= i) {
            UNIT_ASSERT(rsp.Defined());
        } else {
            UNIT_ASSERT(!rsp.Defined());
        }
    }

    UNIT_ASSERT_VALUES_EQUAL(newItemsCounter, 6);
};


Y_UNIT_TEST(HighLoadTestNoTtl) {

    struct TData {
        TData() = default;
        explicit TData(size_t value) : Value(value) {}

        uint64_t GetMemoryUsage() const {
            return (1ull << 12) + RandomNumber(1ull << 12);
        }

        size_t Value = 0;
    };

    using TKey = TMegamindSessionKey;
    using TImdb = TInmemoryLfuKvStorage<TKey, TData>;

    NCachalot::InmemoryStorageSettings settings;
    settings.SetNumberOfBuckets(32);
    settings.SetMaxNumberOfElements(std::numeric_limits<uint64_t>::max());
    settings.SetMaxMemoryUsageBytes(1ull << 30);  // 1 GB
    settings.MutableThreadPoolConfig()->SetNumberOfThreads(8);
    settings.MutableThreadPoolConfig()->SetQueueSize(100);

    auto storage = MakeIntrusive<TImdb>(
        settings,
        &(TMetrics::GetInstance().CacheServiceMetrics.Imdb["UT"])
    );

    constexpr size_t reqs = 1'000'000;
    constexpr size_t keys = 100'000;
    TVector<NThreading::TFuture<TImdb::TEmptyResponse>> setFutures;
    TVector<NThreading::TFuture<TImdb::TSingleRowResponse>> getFutures;

    auto start = TInstant::Now();

    for (size_t i = 0; i < reqs; ++i) {
        if (RandomNumber(3u) % 2) {
            TKey key({.ShardKey = RandomNumber(keys)});
            getFutures.push_back(storage->GetSingleRow(key));
        } else {
            const size_t val = setFutures.size();
            TKey key({.ShardKey = val});
            setFutures.push_back(storage->Set(key, TData(val)));
        }
    }

    size_t tooManySetReqs = 0;
    for (size_t i = 0; i < setFutures.size(); ++i) {
        const auto res = setFutures[i].GetValueSync();
        if (res.Status == EResponseStatus::TOO_MANY_REQUESTS) {
            ++tooManySetReqs;
        }
    }

    bool ok = false;
    size_t tooManyGetReqs = 0;
    for (size_t i = 0; i < getFutures.size(); ++i) {
        const auto res = getFutures[i].GetValueSync();
        if (res.Status == EResponseStatus::OK) {
            UNIT_ASSERT_VALUES_EQUAL(res.Data.GetRef().Value, res.Key.ShardKey);
            ok = true;
        } else if (res.Status == EResponseStatus::TOO_MANY_REQUESTS) {
            ++tooManyGetReqs;
        }
    }

    uint64_t durationMcs = (TInstant::Now() - start).MicroSeconds();

    Cerr << "IMDB: tooManyGetReqs = " << tooManyGetReqs << " (" << Perc(tooManyGetReqs, getFutures.size()) << "%)"
        << " | tooManySetReqs = " << tooManySetReqs << " (" << Perc(tooManySetReqs, setFutures.size()) << "%)"
        << " | duration = " << durationMcs / 1000 << "ms (~" << float(durationMcs) / reqs << "mcs per req)"
        << Endl;

    UNIT_ASSERT(ok);
};

};
