#pragma once

#include <alice/cachalot/library/storage/storage.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <alice/cuttlefish/library/redis/context.h>

#include <util/generic/map.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>
#include <util/thread/pool.h>


namespace NCachalot {


class TRedisPool {
public:
    TRedisPool(const NCachalot::TRedisSettings& settings)
        : Settings(settings)
    { }

    NRedis::TContextPtr GetOrCreateContext();

private:
    NCachalot::TRedisSettings Settings;
    TRWMutex Mut;
    TMap<TThread::TId, NRedis::TContextPtr> Conns;
};


class TRedisStorage : public IKvStorage<TString, TString> {
public:
    using TBase = IKvStorage<TString, TString>;

    using typename TBase::TEmptyResponse;
    using typename TBase::TSingleRowResponse;
    using typename TBase::TMultipleRowsResponse;

public:
    TRedisStorage(const NCachalot::TRedisSettings& settings);

    ~TRedisStorage();

    NThreading::TFuture<TEmptyResponse> Set(
        const TString& key, const TString& data, int64_t ttl = 0, TChroniclerPtr chronicler = nullptr) override;

    NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TString& key, TChroniclerPtr chronicler = nullptr) override;

    NThreading::TFuture<TMultipleRowsResponse> Get(
        const TString& key, TChroniclerPtr chronicler = nullptr) override;

    NThreading::TFuture<TEmptyResponse> Del(
        const TString& key, TChroniclerPtr chronicler = nullptr) override;

    TString GetStorageTag() const override {
        return "redis";
    }

private:
    NCachalot::TRedisSettings Settings;
    TThreadPool ThreadPool;
    TRedisPool Conns;
};


TIntrusivePtr<IKvStorage<TString, TString>> MakeSimpleRedisStorage(const NCachalot::TRedisSettings& settings);

}   // namespace NCachalot
