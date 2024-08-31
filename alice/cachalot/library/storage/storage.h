#pragma once

#include <alice/cachalot/library/debug.h>
#include <alice/cachalot/library/status.h>
#include <alice/cachalot/library/storage/stats.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <ydb/public/sdk/cpp/client/ydb_types/fluent_settings_helpers.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>


namespace NCachalot {

namespace {
    template <typename TExtraDataType>
    class TExtraDataMixin {
    public:
        TExtraDataType ExtraData;
    };

    template <>
    class TExtraDataMixin<void> {
    };
}


template <typename TKeyType, typename TDerivedType, typename TExtraDataType = void>
class TStorageResponseMixin : public TStatus, public TExtraDataMixin<TExtraDataType> {
public:
    using TKey = TKeyType;
    using TDerived = TDerivedType;
    using TExtraData = TExtraDataType;

public:
    TStorageStats Stats;
    TKey Key;

public:
    virtual ~TStorageResponseMixin() = default;

    TDerived& SetKey(TKey key) {
        Key = std::move(key);
        return *This();
    }

    TDerived& SetStatus(EResponseStatus s) {
        TStatus::SetStatus(s);
        return *This();
    }

    TDerived& SetError(TString error) {
        Stats.ErrorMessage = std::move(error);
        return *This();
    }

    TDerived& SetStats(float schedulingTime, float fetchingTime) {
        Stats.SchedulingTime = schedulingTime;
        Stats.FetchingTime = fetchingTime;
        return *This();
    }

    TDerived& SetStats(TStorageStats stats) {
        Stats = std::move(stats);
        return *This();
    }

protected:
    TDerived* This() {
        return dynamic_cast<TDerived*>(this);
    }
};


template <typename TKeyType, typename TExtraDataType = void>
class TEmptyStorageResponse :
    public TStorageResponseMixin<TKeyType, TEmptyStorageResponse<TKeyType, TExtraDataType>, TExtraDataType>
{
public:
    using TKey = TKeyType;
};


template <typename TKeyType, typename TDataType, typename TExtraDataType = void>
class TSingleRowStorageResponse :
    public TStorageResponseMixin<TKeyType, TSingleRowStorageResponse<TKeyType, TDataType, TExtraDataType>, TExtraDataType>
{
public:
    using TKey = TKeyType;
    using TData = TDataType;
    using TExtraData = TExtraDataType;
    using TSelf = TSingleRowStorageResponse<TKey, TData, TExtraData>;

public:
    TMaybe<TData> Data;

public:
    TSelf& SetData(TData data) {
        Data = std::move(data);
        return *this;
    }
};


template <typename TKeyType, typename TDataType, typename TExtraDataType = void>
class TMultipleRowsStorageResponse :
    public TStorageResponseMixin<TKeyType, TMultipleRowsStorageResponse<TKeyType, TDataType, TExtraDataType>, TExtraDataType>
{
public:
    using TKey = TKeyType;
    using TData = TDataType;
    using TExtraData = TExtraDataType;
    using TSelf = TMultipleRowsStorageResponse<TKey, TData, TExtraData>;

public:
    TVector<TData> Rows;

public:
    TSelf& AddData(TData data) {
        Rows.emplace_back(std::move(data));
        return *this;
    }
};


struct TStorageConfig {
    size_t MaxDataSize = 4 * 1024 * 1024;  // 4MB
    size_t MaxRetriesCount = 1;
    TDuration ReadTimeout;
    TDuration WriteTimeout;
    TMaybe<int64_t> DefaultTtlSeconds;
    int64_t MaxTimeToLiveSeconds;
};


/**
 *  @brief base storage class
 */
template <typename TKeyType, typename TDataType, typename TExtraDataType = void>
class IKvStorage : public TThrRefBase {
public:
    using TKey = TKeyType;
    using TData = TDataType;
    using TExtraData = TExtraDataType;
    using TEmptyResponse = TEmptyStorageResponse<TKey, TExtraData>;
    using TSingleRowResponse = TSingleRowStorageResponse<TKey, TData, TExtraData>;
    using TMultipleRowsResponse = TMultipleRowsStorageResponse<TKey, TData, TExtraData>;
    using TSelf = IKvStorage<TKey, TData, TExtraData>;

public:
    virtual ~IKvStorage() = default;

    virtual NThreading::TFuture<TEmptyResponse> Set(
        const TKey& key, const TData& data, int64_t ttl = 0, TChroniclerPtr chronicler = nullptr) = 0;

    virtual NThreading::TFuture<TMultipleRowsResponse> Get(
        const TKey& key, TChroniclerPtr chronicler = nullptr) = 0;

    virtual NThreading::TFuture<TEmptyResponse> SetSingleRow(
        const TKey& key, const TData& data, int64_t ttl = 0, TChroniclerPtr chronicler = nullptr
    ) {
        auto promise = NThreading::NewPromise<TEmptyResponse>();
        auto self = IntrusiveThis();
        Del(key).Subscribe([this, self, promise, key, data, ttl, chronicler] (auto future) mutable {
            TEmptyResponse delRsp = future.ExtractValueSync();
            if (delRsp.Status == EResponseStatus::OK) {
                Set(key, data, ttl, chronicler).Subscribe([self, promise] (auto future) mutable {
                    promise.SetValue(future.ExtractValueSync());
                });
            } else {
                promise.SetValue(delRsp);
            }
        });
        return promise;
    }

    virtual NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TKey& key, TChroniclerPtr chronicler = nullptr) = 0;

    virtual NThreading::TFuture<TEmptyResponse> Del(
        const TKey& key, TChroniclerPtr chronicler = nullptr) = 0;

    virtual TString GetStorageTag() const = 0;

    TStorageConfig* MutableConfig() {
        return &Config;
    }

    TIntrusivePtr<TSelf> IntrusiveThis() {
        return this;
    }

protected:
    TStorageConfig Config;
};


/**
 *  @brief Fake storage. Does nothing.
 */
template <typename TKeyType, typename TDataType, typename TExtraDataType = void>
class TFakeKvStorage : public IKvStorage<TKeyType, TDataType, TExtraDataType> {
public:
    using TBase = IKvStorage<TKeyType, TDataType, TExtraDataType>;

    using TKey = typename TBase::TKey;
    using TData = typename TBase::TData;
    using TExtraData = typename TBase::TExtraData;
    using TEmptyResponse = typename TBase::TEmptyResponse;
    using TSingleRowResponse = typename TBase::TSingleRowResponse;
    using TMultipleRowsResponse = typename TBase::TMultipleRowsResponse;

public:
    virtual NThreading::TFuture<TEmptyResponse> Set(
        const TKey& key, const TData&, int64_t = 0, TChroniclerPtr = nullptr
    ) override {
        return MakeFakeResultFuture<TEmptyResponse>(EResponseStatus::CREATED, key);
    };

    virtual NThreading::TFuture<TMultipleRowsResponse> Get(
        const TKey& key, TChroniclerPtr = nullptr
    ) override {
        return MakeFakeResultFuture<TMultipleRowsResponse>(EResponseStatus::NO_CONTENT, key);
    };

    virtual NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TKey& key, TChroniclerPtr = nullptr
    ) override {
        return MakeFakeResultFuture<TSingleRowResponse>(EResponseStatus::NO_CONTENT, key);
    };

    virtual NThreading::TFuture<TEmptyResponse> Del(
        const TKey& key, TChroniclerPtr = nullptr
    ) override {
        return MakeFakeResultFuture<TEmptyResponse>(EResponseStatus::OK, key);
    };

    virtual TString GetStorageTag() const override {
        return "fake-storage";
    }

private:
    template <typename TResponse>
    static NThreading::TFuture<TResponse> MakeFakeResultFuture(EResponseStatus status, const TKey& key) {
        auto promise = NThreading::NewPromise<TResponse>();
        TResponse rsp;
        rsp.SetKey(key);
        rsp.SetStatus(status);
        promise.SetValue(std::move(rsp));
        return promise.GetFuture();
    }
};


template <typename TKeyType, typename TDataType>
class IMegazordKvStorageCallbacks : public TThrRefBase {
public:
    using TStorage = IKvStorage<TKeyType, TDataType>;
    using TExtraData = typename TStorage::TExtraData;
    using TEmptyResponse = typename TStorage::TEmptyResponse;
    using TSingleRowResponse = typename TStorage::TSingleRowResponse;
    using TMultipleRowsResponse = typename TStorage::TMultipleRowsResponse;

public:
    virtual void OnLocalSetResponse(const TEmptyResponse&, TChroniclerPtr) {};
    virtual void OnGlobalSetResponse(const TEmptyResponse&, TChroniclerPtr) {};
    virtual void OnLocalGetSingleRowResponse(const TSingleRowResponse&, TChroniclerPtr) {};
    virtual void OnGlobalGetSingleRowResponse(const TSingleRowResponse&, TChroniclerPtr) {};
    virtual void OnLocalGetResponse(const TMultipleRowsResponse&, TChroniclerPtr) {};
    virtual void OnGlobalGetResponse(const TMultipleRowsResponse&, TChroniclerPtr) {};
    virtual void OnLocalDelResponse(const TEmptyResponse&, TChroniclerPtr) {};
    virtual void OnGlobalDelResponse(const TEmptyResponse&, TChroniclerPtr) {};
};


template <typename TKeyType, typename TDataType, typename TExtraDataType = void>
class TMegazordKvStorageBase : public IKvStorage<TKeyType, TDataType, TExtraDataType> {
public:
    using TBase = IKvStorage<TKeyType, TDataType, TExtraDataType>;
    using TSelf = TMegazordKvStorageBase<TKeyType, TDataType, TExtraDataType>;

    using TKey = typename TBase::TKey;
    using TData = typename TBase::TData;
    using TExtraData = typename TBase::TExtraData;
    using TEmptyResponse = typename TBase::TEmptyResponse;
    using TSingleRowResponse = typename TBase::TSingleRowResponse;
    using TMultipleRowsResponse = typename TBase::TMultipleRowsResponse;

    using TFakeKvStorage = TFakeKvStorage<TKeyType, TDataType, TExtraDataType>;
    using TStoragePtr = TIntrusivePtr<TBase>;

public:
    #define CALL_BACK_IF_DEFINED(what) \
        if (Callbacks != nullptr) {    \
            Callbacks->what;           \
        }

    TMegazordKvStorageBase(
        TStoragePtr local,
        TStoragePtr global,
        TIntrusivePtr<IMegazordKvStorageCallbacks<TKey, TData>> callbacks = nullptr
    )
        : LocalStorage(local ? std::move(local) : MakeIntrusive<TFakeKvStorage>())
        , GlobalStorage(global ? std::move(global) : MakeIntrusive<TFakeKvStorage>())
        , Callbacks(std::move(callbacks))
    {
    }

    NThreading::TFuture<TEmptyResponse> Set(
        const TKey& key, const TData& data, int64_t defaultTtl = 0, TChroniclerPtr chronicler = nullptr
    ) override {
        auto response = NThreading::NewPromise<TEmptyResponse>();
        auto mngr = NThreading::TSubscriptionManager::NewInstance();

        auto This = TBase::IntrusiveThis();
        mngr->Subscribe(
            WaitBoth(
                LocalStorage->Set(key, data, LocalStorageTtl_.GetOrElse(defaultTtl), chronicler),
                GlobalStorage->Set(key, data, GlobalStorageTtl_.GetOrElse(defaultTtl), chronicler)
            ),
            [this, This, key, response, chronicler](auto future) mutable {
                const auto& [first, second] = future.GetValueSync();

                CALL_BACK_IF_DEFINED(OnLocalSetResponse(first, chronicler));
                CALL_BACK_IF_DEFINED(OnGlobalSetResponse(second, chronicler));

                TEmptyResponse result;
                result.SetKey(key);
                result.SetStatus(SelectFinalSetStatus(first.Status, second.Status));
                response.SetValue(result);
            }
        );

        return response;
    }

    NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TKey& key, TChroniclerPtr chronicler = nullptr
    ) override {
        NThreading::TPromise<TSingleRowResponse> response = NThreading::NewPromise<TSingleRowResponse>();

        auto This = TBase::IntrusiveThis();
        LocalStorage->GetSingleRow(key, chronicler).Subscribe(
            [this, This, response, key, chronicler](auto future) mutable {
                TSingleRowResponse reply = future.ExtractValueSync();
                CALL_BACK_IF_DEFINED(OnLocalGetSingleRowResponse(reply, chronicler));
                if (reply.Status == EResponseStatus::OK) {
                    response.SetValue(std::move(reply));
                } else {
                    GlobalStorage->GetSingleRow(key, chronicler).Subscribe(
                        [this, This, response, chronicler](auto future) mutable {
                            TSingleRowResponse reply = future.ExtractValueSync();
                            CALL_BACK_IF_DEFINED(OnGlobalGetSingleRowResponse(reply, chronicler));
                            response.SetValue(std::move(reply));
                        }
                    );
                }
            }
        );

        return response;
    }

    NThreading::TFuture<TMultipleRowsResponse> Get(
        const TKey& key, TChroniclerPtr chronicler = nullptr
    ) override {
        NThreading::TPromise<TMultipleRowsResponse> response = NThreading::NewPromise<TMultipleRowsResponse>();

        auto This = TBase::IntrusiveThis();
        LocalStorage->Get(key, chronicler).Subscribe(
            [this, This, response, key, chronicler](auto future) mutable {
                TMultipleRowsResponse reply = future.ExtractValueSync();
                CALL_BACK_IF_DEFINED(OnLocalGetResponse(reply, chronicler));
                if (reply.Status == EResponseStatus::OK) {
                    response.SetValue(std::move(reply));
                } else {
                    GlobalStorage->Get(key, chronicler).Subscribe(
                        [this, This, response, chronicler](auto future) mutable {
                            TMultipleRowsResponse reply = future.ExtractValueSync();
                            CALL_BACK_IF_DEFINED(OnGlobalGetResponse(reply, chronicler));
                            response.SetValue(std::move(reply));
                        }
                    );
                }
            }
        );

        return response;
    }

    NThreading::TFuture<TEmptyResponse> Del(
        const TKey& key, TChroniclerPtr chronicler = nullptr
    ) override {
        auto response = NThreading::NewPromise<TEmptyResponse>();
        auto mngr = NThreading::TSubscriptionManager::NewInstance();

        auto This = TBase::IntrusiveThis();
        mngr->Subscribe(
            WaitBoth(
                LocalStorage->Del(key, chronicler),
                GlobalStorage->Del(key, chronicler)
            ),
            [this, This, response, chronicler](auto future) mutable {
                const auto& [first, second] = future.GetValueSync();

                CALL_BACK_IF_DEFINED(OnLocalDelResponse(first, chronicler));
                CALL_BACK_IF_DEFINED(OnGlobalDelResponse(second, chronicler));

                TEmptyResponse result;
                result.SetStatus(SelectFinalDelStatus(first.Status, second.Status));
                response.SetValue(result);
            }
        );

        return response;
    }

    #undef CALL_BACK_IF_DEFINED

    TString GetStorageTag() const override {
        return TStringBuilder() << LocalStorage->GetStorageTag() << '+' << GlobalStorage->GetStorageTag();
    }

protected:
    virtual EResponseStatus SelectFinalSetStatus(EResponseStatus /* local */, EResponseStatus global) const {
        return global;
    }

    virtual EResponseStatus SelectFinalDelStatus(EResponseStatus local, EResponseStatus global) const {
        if (static_cast<int>(local) < static_cast<int>(global)) {
            return global;
        } else {
            return local;
        }
    }

protected:
    TStoragePtr LocalStorage;
    FLUENT_SETTING_OPTIONAL(int64_t, LocalStorageTtl);

    TStoragePtr GlobalStorage;
    FLUENT_SETTING_OPTIONAL(int64_t, GlobalStorageTtl);

    TIntrusivePtr<IMegazordKvStorageCallbacks<TKey, TData>> Callbacks = nullptr;
};

}   // namespace NCachalot
