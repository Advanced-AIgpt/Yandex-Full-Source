#pragma once

// IMDB -- In-Memory DataBase.

#include <alice/cachalot/library/storage/inmemory/fifo_policy.h>
#include <alice/cachalot/library/storage/inmemory/lfu_policy.h>
#include <alice/cachalot/library/storage/inmemory/lru_policy.h>
#include <alice/cachalot/library/storage/inmemory/read_once_policy.h>
#include <alice/cachalot/library/storage/inmemory/utils.h>
#include <alice/cachalot/library/storage/storage.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

#include <atomic>


namespace NCachalot::NPrivate {

/*
    Features of TAssociativeCache:
    * customizable cache policy
    * TTL (common to all items)
    * limited cache size: you can limit memory usage or/and number of elements.

    TAssociativeCache uses TExpirationOrder for expiration control
    and TRemovalPolicy for cache policy enforcing.
    Both members internally store keys in its own order. Hence follows:
    1) When key is removed by cache policy, we must remove it from TExpirationOrder.
    2) When key expires, we must remove it from Policy.
    Yes, this logic adds significant cpu overhead (according to tests),
    but it makes code of every class clear and testable.

    All sub-objects (TExpirationOrder, TRemovalPolicy, TKey, TData, etc)
    must provide methods for estimation of memory usage.
    Total memory usage of all sub-objects and their overheads is computed in TAssociativeCache.
    TAssociativeCache decides to remove unimportant items when memory usage is near to limit.
    Removal of expired items occurs on every Load or Store.
*/



template <typename TItem, typename TClock = TUniqueClock>
class TExpirationOrder {
public:
    using TOrderedStorage = TMap<TInstant, TItem>;
    using TOrderedStorageIterator = typename TOrderedStorage::iterator;

    class TOrderView {
    public:
        class TIterator {
        public:
            explicit TIterator(TOrderedStorageIterator pos)
                : Pos(std::move(pos))
            {
            }

            TIterator& operator++() {
                ++Pos;
                return *this;
            }

            TItem& operator*() {
                return Pos->second;
            }

            bool operator==(const TIterator& other) const {
                return Pos == other.Pos;
            }

        private:
            TOrderedStorageIterator Pos;
        };

    public:
        TOrderView(TOrderedStorageIterator beginPos, TOrderedStorageIterator endPos)
            : Begin(std::move(beginPos))
            , End(std::move(endPos))
        {
        }

        TIterator begin() const {
            return TIterator(Begin);
        }

        TIterator end() const {
            return TIterator(End);
        }

    private:
        TOrderedStorageIterator Begin;
        TOrderedStorageIterator End;
    };

    class TRemovalHelper {
    public:
        TRemovalHelper(TExpirationOrder* orderPtr)
            : OrderPtr(orderPtr)
            , BeginOfPartToRemove(OrderPtr->ExpirationTime2Item.begin())
            , EndOfPartToRemove(OrderPtr->ExpirationTime2Item.upper_bound(OrderPtr->Clock.GetActualTimestamp()))
        {
            Y_ASSERT(OrderPtr != nullptr);
        }

        TOrderView ListExpired() const {
            return TOrderView(BeginOfPartToRemove, EndOfPartToRemove);
        }

        void EraseExpired() {
            for (const TItem& item : ListExpired()) {
                OrderPtr->Item2ExpirationInfo.erase(item);
            }
            OrderPtr->ExpirationTime2Item.erase(BeginOfPartToRemove, EndOfPartToRemove);
        }

    private:
        TExpirationOrder* OrderPtr;
        TOrderedStorageIterator BeginOfPartToRemove;
        TOrderedStorageIterator EndOfPartToRemove;
    };

    friend TRemovalHelper;

    struct TExpirationInfo {
        TOrderedStorageIterator OrderPos;
        TDuration TimeToLive;
    };

public:
    explicit TExpirationOrder()
    {
    }

    void Push(TItem item, const TDuration ttl) {
        // Item should be not present!
        // But in case of broken invariant we keep here removing of duplicate.
        RemoveByPolicy(item);

        TExpirationInfo& expirationInfo = Item2ExpirationInfo[item];  // getting uninitialized object.
        InitializeExpirationInfo(&expirationInfo, std::move(item), ttl);
    }

    void Touch(TItem item) {
        if (const auto pos = Item2ExpirationInfo.find(item); Y_LIKELY(pos != Item2ExpirationInfo.end())) {
            ExpirationTime2Item.erase(pos->second.OrderPos);
            InitializeExpirationInfo(&(pos->second), std::move(item), pos->second.TimeToLive);
        } else {
            // This method should not be called when item is not present in storage.
            Y_ASSERT(false);
        }
    }

    TRemovalHelper GetRemovalHelper() {
        return TRemovalHelper(this);
    }

    void RemoveByPolicy(const TItem& item) {
        if (const auto pos = Item2ExpirationInfo.find(item); Y_LIKELY(pos != Item2ExpirationInfo.end())) {
            ExpirationTime2Item.erase(pos->second.OrderPos);
            Item2ExpirationInfo.erase(pos);
        }
    }

    uint64_t CalcMemoryUsage(const TItem& item) const {
        return GetMemoryUsage(item) * 2 + 64;
    }

private:
    void InitializeExpirationInfo(TExpirationInfo* expirationInfo, TItem item, const TDuration ttl) {
        Y_ASSERT(expirationInfo != nullptr);

        const TInstant expirationTime = Clock.GetActualTimestamp() + ttl;
        const auto emplaceResult = ExpirationTime2Item.emplace(expirationTime, std::move(item));
        Y_ASSERT(emplaceResult.second);  // insertion was successful because key is unique.

        expirationInfo->OrderPos = emplaceResult.first;
        expirationInfo->TimeToLive = ttl;
    }

private:
    TClock Clock;
    TOrderedStorage ExpirationTime2Item;
    THashMap<TItem, TExpirationInfo, THashWithSalt<TItem, 0x33280a04392159abull>> Item2ExpirationInfo;
};


template <typename TRemovalPolicy, typename TKey, typename TData, typename TClock = TUniqueClock>
class TAssociativeCache {
    // Items with least priority by RemovalPolicy are deleted when memory usage is close to the limit.
    // This implementation assumes that MaxMemoryUsage is much bigger than memory usage of one item.

private:
    using TSelf = TAssociativeCache<TRemovalPolicy, TKey, TData>;

public:
    explicit TAssociativeCache(
        uint64_t maxElements,
        uint64_t maxMemoryUsage,
        TDuration ttl = TDuration::Minutes(15),
        bool renewExpirationTimeOnLoad = false
    )
        : MaxElements(maxElements)
        , MaxMemoryUsage(maxMemoryUsage)
        , ExpirationOrder()
        , DefaultTimeToLive(ttl)
        , RenewExpirationTimeOnLoad(renewExpirationTimeOnLoad)
    {
    }

    // Need this for TVector. Because of it MaxElements and MaxMemoryUsage are non-const :(
    TAssociativeCache(const TAssociativeCache&) = default;
    TAssociativeCache(TAssociativeCache&&) = default;

    void Store(TKey key, TData data, TMaybe<TDuration> ttl = Nothing()) {
        RemoveExpiredItems();
        auto pos = Items.find(key);
        if (pos != Items.end()) {
            // Using the same key object to optimize amount of memory used by COW-strings.
            Touch(pos->first);
            CurrentMemoryUsage -= GetMemoryUsage(pos->second);
            CurrentMemoryUsage += GetMemoryUsage(data);
            pos->second = std::move(data);
        } else {
            const uint64_t itemMemUsage = CalcMemoryUsage(key, data);
            ShrinkOnNewItem(itemMemUsage);

            CurrentMemoryUsage += itemMemUsage;
            CurrentNumberOfItems++;

            RemovalPolicy.OnNewItem(key);
            ExpirationOrder.Push(key, ttl.GetOrElse(DefaultTimeToLive));
            Items.emplace(std::move(key), std::move(data));
        }
    }

    TMaybe<TData> Load(const TKey& key) {
        RemoveExpiredItems();

        auto pos = Items.find(key);
        if (pos == Items.end()) {
            return Nothing();
        }

        // Using the same key object to optimize amount of memory used by COW-strings.
        Touch(pos->first);

        return pos->second;
    }

    bool Remove(const TKey& key) {
        RemoveExpiredItems();

        auto pos = Items.find(key);
        if (pos != Items.end()) {
            ExpirationOrder.RemoveByPolicy(key);
            RemoveItem(key);
            return true;
        }

        return false;
    }

    uint64_t GetCurrentMemoryUsage() const {
        return CurrentMemoryUsage;
    }

    uint64_t GetCurrentSize() const {
        return Items.size();
    }

private:
    void Touch(const TKey& key) {
        if (RenewExpirationTimeOnLoad) {
            ExpirationOrder.Touch(key);
        }
        RemovalPolicy.OnTouch(key);
    }

    uint64_t CalcMemoryUsage(const TKey& key, const TData& data) const {
        // 32 is random magic constant.
        // It should express number of bytes used to store keys and datas in Items and Usage2Keys.
        // TODO (paxakor@): calculate some approximation.
        return (
            GetMemoryUsage(data) +
            GetMemoryUsage(key) +
            RemovalPolicy.CalcMemoryUsage(key) +
            ExpirationOrder.CalcMemoryUsage(key) +
            32
        );
    }

    void ShrinkOnNewItem(const uint64_t newItemMemUsage) {
        uint64_t targetCurrentMemoryUsage = 0;
        // 8 is random magic constant.
        if (const uint64_t delta = 8 * newItemMemUsage; delta < MaxMemoryUsage) {
            targetCurrentMemoryUsage = MaxMemoryUsage - delta;
        }

        // Only if new element does not fit, we shrink to targetCurrentMemoryUsage.
        // We don't want to shrink to the targetCurrentMemoryUsage on every Store.
        while (!(
            (newItemMemUsage + GetCurrentMemoryUsage() <= MaxMemoryUsage) &&
            ((Items.size() + 1) <= MaxElements)
        )) {

            TMaybe<TKey> keyToRemove = RemovalPolicy.GetKeyToRemove();

            if (!keyToRemove.Defined()) {
                // No keys left.
                return;
            }

            ExpirationOrder.RemoveByPolicy(*keyToRemove);
            RemoveItem(*keyToRemove);
        }
    }

    void RemoveItem(const TKey& keyToRemove) {
        if (const auto itemPos = Items.find(keyToRemove); itemPos != Items.end()) {
            CurrentMemoryUsage -= CalcMemoryUsage(keyToRemove, itemPos->second);
            CurrentNumberOfItems--;

            Items.erase(itemPos);
        }
    }

    void RemoveExpiredItems() {
        auto expirationHelper = ExpirationOrder.GetRemovalHelper();
        for (const TKey& key : expirationHelper.ListExpired()) {
            RemovalPolicy.RemoveExpiredKey(key);
            RemoveItem(key);
        }
        expirationHelper.EraseExpired();
    }

private:
    /* const */ uint64_t MaxElements = 0;
    /* const */ uint64_t MaxMemoryUsage = 0;

    THashMap<TKey, TData, THashWithSalt<TKey, 0x6a96726bbcae62cdull>> Items;
    TRemovalPolicy RemovalPolicy;
    TExpirationOrder<TKey, TClock> ExpirationOrder;
    /* const */ TDuration DefaultTimeToLive;
    /* const */ bool RenewExpirationTimeOnLoad = false;

    uint64_t CurrentMemoryUsage = 0;  // not precise.
    uint64_t CurrentNumberOfItems = 0;
};


template <typename TMap, typename TKey, typename TData>
class TInmemoryKvStorageImpl {
public:
    TInmemoryKvStorageImpl(
        uint64_t numberOfBuckets,
        uint64_t maxNumberOfElements,
        uint64_t maxMemoryUsageBytes,
        TDuration timeToLive,
        TDuration maxTimeToLive,
        bool renewExpirationTimeOnLoad
    )
        : Buckets(
            numberOfBuckets,
            TMap(
                maxNumberOfElements / numberOfBuckets,
                maxMemoryUsageBytes / numberOfBuckets,
                timeToLive,
                renewExpirationTimeOnLoad
            )
        )
        , Locks(numberOfBuckets)
        , MemoryUsages(numberOfBuckets)
        , NumberOfItems(numberOfBuckets)
        , MaxTimeToLive(maxTimeToLive)
    {
    }

    void Store(TKey key, TData data, TMaybe<TDuration> ttl = Nothing()) {
        const uint64_t bucketId = GetBucketId(key);
        TGuard lock(Locks[bucketId]);
        Buckets[bucketId].Store(std::move(key), std::move(data), ttl);
        MemoryUsages[bucketId].store(Buckets[bucketId].GetCurrentMemoryUsage());
        NumberOfItems[bucketId].store(Buckets[bucketId].GetCurrentSize());
    }

    TMaybe<TData> Load(const TKey& key) {
        const uint64_t bucketId = GetBucketId(key);
        TGuard lock(Locks[bucketId]);
        MemoryUsages[bucketId].store(Buckets[bucketId].GetCurrentMemoryUsage());
        NumberOfItems[bucketId].store(Buckets[bucketId].GetCurrentSize());
        return Buckets[bucketId].Load(key);
    }

    bool Remove(const TKey& key) {
        const uint64_t bucketId = GetBucketId(key);
        TGuard lock(Locks[bucketId]);

        bool isRemoved = Buckets[bucketId].Remove(key);

        MemoryUsages[bucketId].store(Buckets[bucketId].GetCurrentMemoryUsage());
        NumberOfItems[bucketId].store(Buckets[bucketId].GetCurrentSize());

        return isRemoved;
    }

    uint64_t GetCurrentMemoryUsage() {
        uint64_t sum = 0;
        for (const std::atomic<uint64_t>& val : MemoryUsages) {
            sum += val.load();
        }
        return sum;
    }

    uint64_t GetNumberOfItems() {
        uint64_t sum = 0;
        for (const std::atomic<uint64_t>& val : NumberOfItems) {
            sum += val.load();
        }
        return sum;
    }

    TDuration GetMaxTimeToLive() {;
        return MaxTimeToLive;
    }

private:
    uint64_t GetBucketId(const TKey& key) const {
        return THashWithSalt<TKey, 0x3e848aa714d5fc1cull>()(key) % Buckets.size();
    }

private:
    TVector<TMap> Buckets;
    TVector<TMutex> Locks;
    TVector<std::atomic<uint64_t>> MemoryUsages;
    TVector<std::atomic<uint64_t>> NumberOfItems;
    TDuration MaxTimeToLive;
};


}  // namespace NCachalot::NPrivate


namespace NCachalot {


template<typename TKey, typename TData, typename TClock = TUniqueClock>
using TFifoCache = NPrivate::TAssociativeCache<NPrivate::TFifoPolicy<TKey>, TKey, TData, TClock>;

template<typename TKey, typename TData, typename TClock = TUniqueClock>
using TLfuCache = NPrivate::TAssociativeCache<NPrivate::TLfuPolicy<TKey>, TKey, TData, TClock>;

template<typename TKey, typename TData, typename TClock = TUniqueClock>
using TLruCache = NPrivate::TAssociativeCache<NPrivate::TLruPolicy<TKey>, TKey, TData, TClock>;

template<typename TKey, typename TData, typename TClock = TUniqueClock>
using TReadOnceCache = NPrivate::TAssociativeCache<NPrivate::TReadOncePolicy<TKey>, TKey, TData, TClock>;


template <typename TCache, typename TKeyType, typename TDataType, typename TExtraDataType = void>
class TInmemoryKvStorage : public IKvStorage<TKeyType, TDataType, TExtraDataType> {
public:
    using TBase = IKvStorage<TKeyType, TDataType, TExtraDataType>;
    using TSelf = TInmemoryKvStorage<TKeyType, TDataType, TExtraDataType>;

    using TKey = typename TBase::TKey;
    using TData = typename TBase::TData;
    using TExtraData = typename TBase::TExtraData;
    using TEmptyResponse = typename TBase::TEmptyResponse;
    using TSingleRowResponse = typename TBase::TSingleRowResponse;
    using TMultipleRowsResponse = typename TBase::TMultipleRowsResponse;

public:
    TInmemoryKvStorage(
        const NCachalot::TInmemoryStorageSettings& settings,
        TInmemoryStorageMetrics* metrics
    )
        : Metrics(metrics)
        , StorageImpl(
            settings.NumberOfBuckets(),
            settings.MaxNumberOfElements(),
            settings.MaxMemoryUsageBytes(),
            TDuration::Seconds(settings.TimeToLiveSeconds()),
            TDuration::Seconds(settings.MaxTimeToLiveSeconds()),
            settings.RenewExpirationTimeOnLoad()
        )
    {
        Y_ENSURE(Metrics != nullptr);
        TBase::MutableConfig()->DefaultTtlSeconds = settings.TimeToLiveSeconds();
        ThreadPool.Start(
            settings.ThreadPoolConfig().NumberOfThreads(),
            settings.ThreadPoolConfig().QueueSize()
        );
    }

    NThreading::TFuture<TEmptyResponse> Set(
        const TKey& key, const TData& data, int64_t ttlSeconds = 0, TChroniclerPtr chronicler = nullptr
    ) override {
        Metrics->SetMemoryUsage(StorageImpl.GetCurrentMemoryUsage());
        Metrics->SetNumberOfItems(StorageImpl.GetNumberOfItems());

        if (TBase::Config.MaxDataSize < GetMemoryUsage(data)) {
            TEmptyResponse response;
            response.SetKey(key);
            response.SetError("Data is too large");
            response.SetStatus(EResponseStatus::BAD_REQUEST);
            return NThreading::MakeFuture(std::move(response));
        }

        const auto startTime = chronicler ? chronicler->GetStartTime() : TInstant::Now();
        auto promise = NThreading::NewPromise<TEmptyResponse>();
        auto self = TBase::IntrusiveThis();
        TMaybe<TDuration> ttlMaybe;
        if (ttlSeconds <= 0) {
            ttlMaybe = Nothing();
        } else if (TDuration::Seconds(ttlSeconds) > StorageImpl.GetMaxTimeToLive()) {
            ttlMaybe = StorageImpl.GetMaxTimeToLive();
        } else {
            ttlMaybe = TDuration::Seconds(ttlSeconds);
        }

        const bool added = ThreadPool.AddFunc(
            [this, self, key, data, startTime, promise, ttlMaybe] () mutable {
                StorageImpl.Store(key, std::move(data), ttlMaybe);

                TEmptyResponse rsp;
                rsp.SetKey(std::move(key));
                rsp.SetStatus(EResponseStatus::OK);
                Metrics->Set.OnOk(startTime);
                promise.SetValue(std::move(rsp));
            }
        );

        if (!added) {
            return Make429<TEmptyResponse>(key);
        }

        return promise;
    }

    NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TKey& key, TChroniclerPtr chronicler = nullptr
    ) override {
        Metrics->SetMemoryUsage(StorageImpl.GetCurrentMemoryUsage());
        Metrics->SetNumberOfItems(StorageImpl.GetNumberOfItems());

        const auto startTime = chronicler ? chronicler->GetStartTime() : TInstant::Now();
        auto promise = NThreading::NewPromise<TSingleRowResponse>();
        auto self = TBase::IntrusiveThis();
        const bool added = ThreadPool
            .AddFunc([this, self, key, startTime, promise] () mutable {
                TSingleRowResponse rsp;

                if (TMaybe<TData> data = StorageImpl.Load(key)) {
                    rsp.SetData(data.GetRef());
                    rsp.SetStatus(EResponseStatus::OK);
                    Metrics->Get.OnOk(startTime);
                } else {
                    rsp.SetStatus(EResponseStatus::NO_CONTENT);
                    Metrics->Get.OnNotFound(startTime);
                }
                rsp.SetKey(std::move(key));
                promise.SetValue(std::move(rsp));
            }
        );

        if (!added) {
            return Make429<TSingleRowResponse>(key);
        }

        return promise;
    }

    NThreading::TFuture<TMultipleRowsResponse> Get(
        const TKey& key, TChroniclerPtr /* chronicler */ = nullptr
    ) override {
        TMultipleRowsResponse response;
        response.SetKey(key);
        response.SetError("IStorage::Get(MultipleRows) method is not implemeted for TInmemoryKvStorage");
        response.SetStatus(EResponseStatus::NOT_IMPLEMENTED);
        return NThreading::MakeFuture(response);
    }

    NThreading::TFuture<TEmptyResponse> Del(
        const TKey& key, TChroniclerPtr chronicler = nullptr
    ) override {
        const auto startTime = chronicler ? chronicler->GetStartTime() : TInstant::Now();
        auto promise = NThreading::NewPromise<TEmptyResponse>();
        auto self = TBase::IntrusiveThis();

        const bool enqueued = ThreadPool.AddFunc([this, self, key, startTime, promise]() mutable {
            TEmptyResponse response;

            if (StorageImpl.Remove(key)) {
                response.SetStatus(EResponseStatus::OK);
                Metrics->Delete.OnOk(startTime);
            } else {
                response.SetStatus(EResponseStatus::NO_CONTENT);
                Metrics->Delete.OnNotFound(startTime);
            }

            response.SetKey(std::move(key));
            promise.SetValue(std::move(response));
        });

        if (!enqueued) {
            return Make429<TEmptyResponse>(key);
        }

        Metrics->SetMemoryUsage(StorageImpl.GetCurrentMemoryUsage());
        Metrics->SetNumberOfItems(StorageImpl.GetNumberOfItems());

        return promise;
    }

    TString GetStorageTag() const override {
        if constexpr (std::is_same_v<TCache, TFifoCache<TKey, TData>>) {
            return "'in-memory fifo storage'";
        } else if constexpr (std::is_same_v<TCache, TLfuCache<TKey, TData>>) {
            return "'in-memory lfu storage'";
        } else if constexpr (std::is_same_v<TCache, TLruCache<TKey, TData>>) {
            return "'in-memory lru storage'";
        } else if constexpr (std::is_same_v<TCache, TReadOnceCache<TKey, TData>>) {
            return "'in-memory read_once storage'";
        } else {
            static_assert(std::is_same_v<TCache, void>);  // fail compilation
        }
    }

private:
    template<typename TResult>
    NThreading::TFuture<TResult> Make429(const TKey& key) {
        if constexpr (std::is_same_v<TResult, TEmptyResponse>) {  // sorry, I'm lazy
            Metrics->Set.OnQueueFull();
        } else {
            Metrics->Get.OnQueueFull();
        }

        TResult response;
        response.SetKey(key);
        response.SetError("Queue is full");
        response.SetStatus(EResponseStatus::TOO_MANY_REQUESTS);
        return NThreading::MakeFuture(std::move(response));
    }

private:
    TThreadPool ThreadPool;

    TInmemoryStorageMetrics* Metrics;

    NPrivate::TInmemoryKvStorageImpl<TCache, TKey, TData> StorageImpl;
};


template<typename TKey, typename TData>
using TInmemoryFifoKvStorage = TInmemoryKvStorage<TFifoCache<TKey, TData>, TKey, TData>;

template<typename TKey, typename TData>
using TInmemoryLfuKvStorage = TInmemoryKvStorage<TLfuCache<TKey, TData>, TKey, TData>;

template<typename TKey, typename TData>
using TInmemoryLruKvStorage = TInmemoryKvStorage<TLruCache<TKey, TData>, TKey, TData>;

template<typename TKey, typename TData>
using TInmemoryReadOnceKvStorage = TInmemoryKvStorage<TReadOnceCache<TKey, TData>, TKey, TData>;


template <typename TKeyType, typename TDataType>
TIntrusivePtr<IKvStorage<TKeyType, TDataType>> MakeInmemoryStorage(
    const NCachalot::TInmemoryStorageSettings& settings,
    TInmemoryStorageMetrics* metrics
) {
    if (!settings.Enabled()) {
        return nullptr;
    }

    Y_ENSURE(settings.HasNumberOfBuckets());
    Y_ENSURE(settings.HasMaxNumberOfElements());
    Y_ENSURE(settings.HasMaxMemoryUsageBytes());
    Y_ENSURE(settings.HasRemovalPolicy());
    Y_ENSURE(settings.HasThreadPoolConfig());
    Y_ENSURE(settings.HasTimeToLiveSeconds());
    Y_ENSURE(metrics != nullptr);

    switch (settings.RemovalPolicy()) {
        case NCachalot::InmemoryStorageSettings::FIFO:
            return MakeIntrusive<TInmemoryFifoKvStorage<TKeyType, TDataType>>(settings, metrics);

        case NCachalot::InmemoryStorageSettings::LRU:
            return MakeIntrusive<TInmemoryLruKvStorage<TKeyType, TDataType>>(settings, metrics);

        case NCachalot::InmemoryStorageSettings::LFU:
            return MakeIntrusive<TInmemoryLfuKvStorage<TKeyType, TDataType>>(settings, metrics);

        case NCachalot::InmemoryStorageSettings::READ_ONCE:
            return MakeIntrusive<TInmemoryReadOnceKvStorage<TKeyType, TDataType>>(settings, metrics);

        default:
            Y_ENSURE(!"Unsupported RemovalPolicy");
    }

    return nullptr;
}

}   // namespace NCachalot
