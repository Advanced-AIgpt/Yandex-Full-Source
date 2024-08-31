#pragma once

#include <alice/cachalot/library/storage/storage.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>

#include <type_traits>


namespace NCachalot {

template <typename TResponse>
auto MakeAsyncResponse(TResponse response) {
    NThreading::TPromise<TResponse> promise = NThreading::NewPromise<TResponse>();
    promise.SetValue(std::move(response));
    return promise;
}


template <typename TKeyType, typename TDataType, typename TExtraDataType = void>
class TKvStorageMock : public IKvStorage<TKeyType, TDataType, TExtraDataType> {
public:
    using TBase = IKvStorage<TKeyType, TDataType, TExtraDataType>;

    using typename TBase::TKey;
    using typename TBase::TData;
    using typename TBase::TEmptyResponse;
    using typename TBase::TSingleRowResponse;
    using typename TBase::TMultipleRowsResponse;

public:
    NThreading::TFuture<TEmptyResponse> Set(
        const TKey& key, const TData& data, int64_t ttlSeconds = 0, TChroniclerPtr /* chronicler */ = nullptr
    ) override {
        Clenup();

        if constexpr (std::is_same_v<TData, TString>) {
            // Check size only for cache-set requests
            if (data.size() > TBase::Config.MaxDataSize) {
                return MakeAsyncResponse(
                    TEmptyResponse()
                        .SetKey(key)
                        .SetError(TStringBuilder() << "Record size (" << data.size() << ") exceeds the limit.")
                        .SetStatus(EResponseStatus::BAD_REQUEST)
                );
            }
        }

        {
            TWriteGuard lock(Mutex);
            SetImpl(key, data, ttlSeconds);
        }

        return MakeAsyncResponse(
            TEmptyResponse()
                .SetKey(key)
                .SetStatus(EResponseStatus::CREATED)
        );
    }

    void SetImpl(const TKey& key, const TData& data, int64_t ttlSeconds) {
        Table[key].emplace_back(data, GetExpirationTime(ttlSeconds));
    }

    NThreading::TFuture<TEmptyResponse> SetSingleRow(
        const TKey& key, const TData& data, int64_t ttlSeconds = 0, TChroniclerPtr /* chronicler */ = nullptr
    ) override {
        Clenup();

        if constexpr (std::is_same_v<TData, TString>) {
            // Check size only for cache-set requests
            if (data.size() > TBase::Config.MaxDataSize) {
                return MakeAsyncResponse(
                    TEmptyResponse()
                        .SetKey(key)
                        .SetError(TStringBuilder() << "Record size (" << data.size() << ") exceeds the limit.")
                        .SetStatus(EResponseStatus::BAD_REQUEST)
                );
            }
        }

        {
            TWriteGuard lock(Mutex);
            SetSingleRowImpl(key, data, ttlSeconds);
        }

        return MakeAsyncResponse(
            TEmptyResponse()
                .SetKey(key)
                .SetStatus(EResponseStatus::CREATED)
        );
    }

    void SetSingleRowImpl(const TKey& key, const TData& data, int64_t ttlSeconds) {
        if (Table[key].size()) {
            Table[key].clear();
        }
        Table[key].emplace_back(data, GetExpirationTime(ttlSeconds));
    }

    template<typename TPedicat>
    void Upsert(const TKey& key, const TData& data, int64_t ttlSeconds, const TPedicat& selector) {
        Clenup();

        {
            TWriteGuard lock(Mutex);

            for (auto& row : Table[key]) {
                if (selector(row.Data)) {
                    row = TTableRow(data, GetExpirationTime(ttlSeconds));
                    return;
                }
            }
            SetImpl(key, data, ttlSeconds);
        }
    }

    bool InsertUnique(const TKey& key, const TData& data, int64_t ttlSeconds = 0) {
        Clenup();

        if (TWriteGuard lock(Mutex); Table[key].empty()) {
            Table[key].emplace_back(data, GetExpirationTime(ttlSeconds));
            return true;
        }

        return false;
    }

    bool Contains(const TKey& key) {
        Clenup();

        TWriteGuard lock(Mutex);
        return !Table[key].empty();
    }

    NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TKey& key, TChroniclerPtr /* chronicler */ = nullptr
    ) override {
        Clenup();

        TSingleRowResponse response;
        response.SetKey(key);

        if (TReadGuard lock(Mutex); Table.contains(key) && Table.at(key).size() == 1) {
            response.SetData(Table.at(key)[0].Data);
            response.SetStatus(EResponseStatus::OK);
        } else {
            response.SetStatus(EResponseStatus::NO_CONTENT);
            response.SetError("Key not found in mock storage");
        }

        return MakeAsyncResponse(std::move(response));
    }

    NThreading::TFuture<TMultipleRowsResponse> Get(
        const TKey& key, TChroniclerPtr /* chronicler */ = nullptr
    ) override {
        Clenup();

        TMultipleRowsResponse response;
        response.SetKey(key);
        response.SetStatus(EResponseStatus::OK);

        if (TReadGuard lock(Mutex); Table.contains(key)) {
            for (const TTableRow& row : Table.at(key)) {
                response.AddData(row.Data);
            }
        }

        return MakeAsyncResponse(std::move(response));
    }

    NThreading::TFuture<TEmptyResponse> Del(
        const TKey& key, TChroniclerPtr /* chronicler */ = nullptr
    ) override {
        {
            TWriteGuard lock(Mutex);
            Table.erase(key);
        }

        return MakeAsyncResponse(
            TEmptyResponse()
                .SetKey(key)
                .SetStatus(EResponseStatus::OK)
        );
    }

    template<typename TPedicat>
    void DelIf(const TKey& key, const TPedicat& pedicat) {
        TWriteGuard lock(Mutex);
        TVector<TTableRow>& rows = Table[key];
        rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const TTableRow& row) {
            return pedicat(row.Data);
        }), rows.end());
    }

    TString GetStorageTag() const override {
        return "mock";
    }

private:
    struct TTableRow {
        TTableRow(TData data, TInstant expirationTime)
            : Data(std::move(data))
            , ExpirationTime(std::move(expirationTime))
        {
        }

        TTableRow(const TTableRow&) = default;
        TTableRow(TTableRow&&) = default;

        TTableRow& operator=(const TTableRow&) = default;
        TTableRow& operator=(TTableRow&&) = default;

        TData Data;
        TInstant ExpirationTime;
    };

    static TInstant MakeFutureInstant(int64_t delta) {
        return TInstant::Now() + TDuration::Seconds(delta);
    }

    TInstant GetExpirationTime(int64_t ttlSeconds) {
        if (ttlSeconds > 0) {
            return MakeFutureInstant(ttlSeconds);
        }
        if (TBase::Config.DefaultTtlSeconds.Defined()) {
            return MakeFutureInstant(TBase::Config.DefaultTtlSeconds.GetRef());
        }
        return TInstant::Max();
    }

    void Clenup() {
        TWriteGuard lock(Mutex);
        const TInstant now = TInstant::Now();
        for (auto& [key, rows] : Table) {
            EraseIf(rows, [now](const TTableRow& row) {
                return row.ExpirationTime < now;
            });
        }
    }

private:
    TRWMutex Mutex;
    THashMap<TKey, TVector<TTableRow>> Table;
};

template <typename TStorage, typename... TArgs>
TIntrusivePtr<IKvStorage<typename TStorage::TKey, typename TStorage::TData, typename TStorage::TExtraData>> MakeStorage(
    bool isFake,
    TArgs&&... args
) {
    if (isFake) {
        return MakeIntrusive<TKvStorageMock<typename TStorage::TKey, typename TStorage::TData,
                                            typename TStorage::TExtraData>>();
    }
    return MakeIntrusive<TStorage>(std::forward<TArgs>(args)...);
}

}   // namespace NCachalot
