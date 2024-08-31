#pragma once

#include <alice/cachalot/library/storage/inmemory/imdb.h>
#include <alice/cachalot/library/storage/redis.h>
#include <alice/cachalot/library/storage/storage.h>
#include <alice/cachalot/library/storage/ydb.h>

#include <util/ysaveload.h>


namespace NCachalot {

struct TMegamindSessionKey {
    ui64 ShardKey = 0;
    TString Key;

    bool operator==(const NCachalot::TMegamindSessionKey& rhs) const;
    uint64_t GetMemoryUsage() const;
};

struct TMegamindSessionData {
    TString Puid;
    TString Data;

    Y_SAVELOAD_DEFINE(Puid, Data);

    TString ToString() const;
    uint64_t GetMemoryUsage() const;

    static TMegamindSessionData FromString(const TString&);
};

class TMegamindSessionYdbStorage : public IKvStorage<TMegamindSessionKey, TMegamindSessionData> {
public:
    using TBase = IKvStorage<TMegamindSessionKey, TMegamindSessionData>;

    using typename TBase::TEmptyResponse;
    using typename TBase::TSingleRowResponse;
    using typename TBase::TMultipleRowsResponse;

public:
    TMegamindSessionYdbStorage(const NCachalot::TYdbSettings& settings);

    NThreading::TFuture<TEmptyResponse> Set(const TMegamindSessionKey& key,
        const TMegamindSessionData& data, int64_t ttl = 0, TChroniclerPtr /* chronicler */ = nullptr) override;

    NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TMegamindSessionKey& key, TChroniclerPtr /* chronicler */ = nullptr) override;

    NThreading::TFuture<TMultipleRowsResponse> Get(
        const TMegamindSessionKey& key, TChroniclerPtr /* chronicler */ = nullptr) override;

    NThreading::TFuture<TEmptyResponse> Del(
        const TMegamindSessionKey& key, TChroniclerPtr /* chronicler */ = nullptr) override;

    TString GetStorageTag() const override {
        return "mm-session-ydb";
    }

private:
    TIntrusivePtr<TYdbContext> Ydb;
};


class TMegamindSessionStorage :
    public TMegazordKvStorageBase<TMegamindSessionKey, TMegamindSessionData>
{
public:
    using TBase = TMegazordKvStorageBase<TMegamindSessionKey, TMegamindSessionData>;
    using TBase::TBase;

    TBase::TStoragePtr GetLocalStorage() const;
};

}   // namespace NCachalot


template<>
struct THash<NCachalot::TMegamindSessionKey> {
    ui64 operator()(const NCachalot::TMegamindSessionKey& key) const {
        ui64 res = THash<ui64>{}(key.ShardKey);
        return CombineHashes(res, THash<TString>{}(key.Key));
    }
};
