#pragma once

#include <alice/cachalot/library/storage/ydb.h>
#include <alice/cachalot/library/storage/storage.h>

#include <util/digest/multi.h>

namespace NCachalot {
    struct TVinsContextKey {
        TVinsContextKey() = default;
        TVinsContextKey(const NCachalotProtocol::TVinsContextKey&);
        void Normalize();  // tolower all strings
        void ComputeShardKey();
        void LoadFrom(const NCachalotProtocol::TVinsContextKey&);
        void SaveTo(NCachalotProtocol::TVinsContextKey&);
        bool IsFull() const;

        ui64 ShardKey = 0;
        TString Key;
        // additional field. Can be used instead of Key. TODO: do smth better
        TString Puid;
    };

    struct TVinsExtraData {
        size_t NumDeletedRows = 0;
    };

    class TVinsContextStorage: public IKvStorage<TVinsContextKey, TString, TVinsExtraData> {
    public:
        using TBase = IKvStorage<TVinsContextKey, TString, TVinsExtraData>;

        using typename TBase::TEmptyResponse;
        using typename TBase::TMultipleRowsResponse;
        using typename TBase::TSingleRowResponse;

    public:
        TVinsContextStorage(const NCachalot::TYdbSettings& settings);

        NThreading::TFuture<TEmptyResponse> Set(
            const TVinsContextKey& key, const TString& data, int64_t ttl = 0,
            TChroniclerPtr /* chronicler */ = nullptr) override;

        NThreading::TFuture<TSingleRowResponse> GetSingleRow(
            const TVinsContextKey& key, TChroniclerPtr /* chronicler */ = nullptr) override;

        NThreading::TFuture<TMultipleRowsResponse> Get(
            const TVinsContextKey& key, TChroniclerPtr /* chronicler */ = nullptr) override;

        NThreading::TFuture<TEmptyResponse> Del(
            const TVinsContextKey& key, TChroniclerPtr /* chronicler */ = nullptr) override;

        TString GetStorageTag() const override {
            return "vins-context-ydb";
        }

    private:
        TIntrusivePtr<TYdbContext> Ydb;
        const TDuration ReadTimeout;
        const TDuration WriteTimeout;
    };

}

template <>
struct THash<NCachalot::TVinsContextKey> {
    ui64 operator()(const NCachalot::TVinsContextKey& key) const {
        ui64 res = THash<ui64>{}(key.ShardKey);
        return MultiHash(res, THash<TString>{}(key.Key));
    }
};

template <>
struct TEqualTo<NCachalot::TVinsContextKey> {
    bool operator()(const NCachalot::TVinsContextKey& lhs, const NCachalot::TVinsContextKey& rhs) const {
        return lhs.ShardKey == rhs.ShardKey && lhs.Key == rhs.Key;
    }
};
