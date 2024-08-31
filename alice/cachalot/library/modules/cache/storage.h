#pragma once

#include <alice/cachalot/library/storage/storage.h>
#include <alice/cachalot/library/storage/ydb.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>
#include <alice/cachalot/events/cachalot.ev.pb.h>

namespace NCachalot {


using TSimpleKvStorage = IKvStorage<TString, TString>;
using TSimpleEmptyResponse = TEmptyStorageResponse<TString>;
using TSimpleSingleRowResponse = TSingleRowStorageResponse<TString, TString>;
using TSimpleMultipleRowsResponse = TMultipleRowsStorageResponse<TString, TString>;


class TYdbStorage : public IKvStorage<TString, TString> {
public:
    using TBase = IKvStorage<TString, TString>;

    using typename TBase::TEmptyResponse;
    using typename TBase::TSingleRowResponse;
    using typename TBase::TMultipleRowsResponse;

    static const int64_t DEFAULT_TTL_SECONDS = 12 * 60 * 60;    // 12 hours

    TYdbStorage(const TYdbSettings& settings, TCacheYdbOperationSettings operationSettings);

    ~TYdbStorage();

    NThreading::TFuture<TEmptyResponse> Set(
        const TString& key, const TString& data, int64_t ttl = DEFAULT_TTL_SECONDS, TChroniclerPtr chronicler = nullptr
    ) override;

    NThreading::TFuture<TSingleRowResponse> GetSingleRow(
        const TString& key, TChroniclerPtr chronicler = nullptr
    ) override;

    NThreading::TFuture<TMultipleRowsResponse> Get(const TString& key, TChroniclerPtr chronicler = nullptr) override;

    NThreading::TFuture<TEmptyResponse> Del(const TString& key, TChroniclerPtr chronicler = nullptr) override;

    TString GetStorageTag() const override {
        return "ydb";
    }

private:
    TYdbContext Ydb;
    TCacheYdbOperationSettings OperationSettings;
};


class TCacheStorage : public TMegazordKvStorageBase<TString, TString> {
public:
    using TBase = TMegazordKvStorageBase<TString, TString>;
    using TEmptyResponse = typename TBase::TEmptyResponse;
    using TSingleRowResponse = typename TBase::TSingleRowResponse;
    using TStoragePtr = typename TBase::TStoragePtr;

public:
    TCacheStorage(
        TStoragePtr local, TStoragePtr global,
        uint64_t localTtl, uint64_t globalTtl,
        TIntrusivePtr<IMegazordKvStorageCallbacks<TString, TString>> callbacks
    )
        : TBase(std::move(local), std::move(global), std::move(callbacks))
    {
        TBase::LocalStorageTtl(localTtl);
        TBase::GlobalStorageTtl(globalTtl);
    }
};


TIntrusivePtr<TSimpleKvStorage> MakeSimpleYdbStorage(
    const NCachalot::TYdbSettings& settings,
    TCacheYdbOperationSettings operationSettings
);
TIntrusivePtr<TSimpleKvStorage> MakeSimpleImdbStorage(
    const NCachalot::TInmemoryStorageSettings& settings,
    TInmemoryStorageMetrics* metrics);

}   // namespace NCachalot
