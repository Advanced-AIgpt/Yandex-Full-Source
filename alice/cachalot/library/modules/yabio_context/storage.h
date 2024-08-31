#pragma once

#include <alice/cachalot/library/storage/ydb.h>
#include <alice/cachalot/library/storage/storage.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <util/digest/multi.h>


namespace NCachalot {

    struct TYabioContextKey {
    public:
        TYabioContextKey() = default;
        explicit TYabioContextKey(const NCachalotProtocol::TYabioContextKey&);

        void SaveTo(NCachalotProtocol::TYabioContextKey&) const;
        bool IsFull() const;
        TString AsString() const;

    private:
        void Normalize();  // tolower all strings
        void ComputeShardKey();

    public:
        ui64 ShardKey = 0;
        TString GroupId;
        TString DevModel;
        TString DevManuf;
    };


    class TYabioContextYdbStorage : public IKvStorage<TYabioContextKey, TString> {
    public:
        using TBase = IKvStorage<TYabioContextKey, TString>;

        using typename TBase::TEmptyResponse;
        using typename TBase::TSingleRowResponse;
        using typename TBase::TMultipleRowsResponse;

        static const int64_t DEFAULT_TTL_SECONDS = -1;    // irrelevant

        explicit TYabioContextYdbStorage(const TYabioContextStorageSettings& settings);

        NThreading::TFuture<TEmptyResponse> Set(
            const TYabioContextKey& key,
            const TString& data,
            int64_t ttl = DEFAULT_TTL_SECONDS,
            TChroniclerPtr chronicler = nullptr
        ) override;

        NThreading::TFuture<TSingleRowResponse> GetSingleRow(
            const TYabioContextKey& key,
            TChroniclerPtr chronicler = nullptr
        ) override;

        NThreading::TFuture<TMultipleRowsResponse> Get(
            const TYabioContextKey& key,
            TChroniclerPtr chronicler = nullptr
        ) override;

        NThreading::TFuture<TEmptyResponse> Del(
            const TYabioContextKey& key,
            TChroniclerPtr chronicler = nullptr
        ) override;

        TString GetStorageTag() const override {
            return "yabio-context-ydb";
        }

    private:
        TYdbContext Ydb;
        TYabioContextStorageSettings Settings;
    };


    TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> MakeYabioContextYdbStorage(
        const NCachalot::TYabioContextStorageSettings& settings
    );

}  // namespace NCachalot


template <>
struct THash<NCachalot::TYabioContextKey> {
    ui64 operator()(const NCachalot::TYabioContextKey& key) const {
        return MultiHash(
            THash<ui64>{}(key.ShardKey),
            THash<TString>{}(key.GroupId),
            THash<TString>{}(key.DevModel),
            THash<TString>{}(key.DevManuf)
        );
    }
};

template <>
struct TEqualTo<NCachalot::TYabioContextKey> {
    bool operator()(const NCachalot::TYabioContextKey& lhs, const NCachalot::TYabioContextKey& rhs) const {
        return (
            lhs.ShardKey == rhs.ShardKey &&
            lhs.GroupId == rhs.GroupId &&
            lhs.DevModel == rhs.DevModel &&
            lhs.DevManuf == rhs.DevManuf
        );
    }
};
