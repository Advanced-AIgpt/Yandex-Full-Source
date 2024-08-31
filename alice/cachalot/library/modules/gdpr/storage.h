#pragma once

#include <alice/cachalot/library/storage/ydb.h>
#include <alice/cachalot/library/storage/storage.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <util/digest/multi.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>

namespace NCachalot {

struct TGDPRKey {
    TGDPRKey() = default;
    void ComputeShardKey();

    bool operator==(const TGDPRKey& other) const {
        return Puid == other.Puid;
    }

    ui64 ShardKey = 0;
    TString Puid;
};

struct TServiceStatus {
    TString Service;
    TString Status;
    TString Timestamp;
};

struct TGDPRData {
    TVector<TServiceStatus> Statuses;
};

struct TGDPRGetUserDataResponse {
    TString Puid;
    TGDPRData Data;
};

struct TGDPRGetRequestsResponse {
    int Limit = 0;
    int Offset = 0;
    // puid: data
    TMap<TString, TGDPRData> PersonalizedData;
};

struct TGDPRSetUserDataResponse {
};

using TSingleRowResponse = TSingleRowStorageResponse<TGDPRKey, TGDPRGetUserDataResponse>;
using TMultipleRowsResponse = TSingleRowStorageResponse<TGDPRKey, TGDPRGetRequestsResponse>;
using TSingleRowSetResponse = TSingleRowStorageResponse<TGDPRKey, TGDPRSetUserDataResponse>;
using TAsyncGDPRGetUserDataResponse = NThreading::TFuture<TSingleRowResponse>;
using TAsyncGDPRGetRequestsResponse = NThreading::TFuture<TMultipleRowsResponse>;
using TAsyncGDPRSetUserDataResponse = NThreading::TFuture<TSingleRowSetResponse>;
using TPromiseGDPRSetUserDataResponse = NThreading::TPromise<TSingleRowSetResponse>;

class IGDPRStorage: public TThrRefBase {
public:
    virtual TAsyncGDPRGetUserDataResponse GetUserData(const TGDPRKey& key) = 0;
    virtual TAsyncGDPRGetRequestsResponse GetRequests(int limit, int offset) = 0;
    virtual TAsyncGDPRSetUserDataResponse SetUserData(const TGDPRKey& key, const TGDPRData& data) = 0;
    virtual TString GetStorageTag() const = 0;
};

class TGDPRYdbStorage: public IGDPRStorage {
public:
    TGDPRYdbStorage(const NCachalot::TYdbSettings& settings);

    TAsyncGDPRGetUserDataResponse GetUserData(const TGDPRKey& key) override;
    TAsyncGDPRGetRequestsResponse GetRequests(int limit, int offset) override;
    TAsyncGDPRSetUserDataResponse SetUserData(const TGDPRKey& key, const TGDPRData& data) override;
    TString GetStorageTag() const override {
        return "gdpr-ydb";
    }
private:
    TYdbContext Ydb;
};

class TGDPRStorageMock: public IGDPRStorage {
public:
    TGDPRStorageMock();

    TAsyncGDPRGetUserDataResponse GetUserData(const TGDPRKey& key) override;
    TAsyncGDPRGetRequestsResponse GetRequests(int limit, int offset) override;
    TAsyncGDPRSetUserDataResponse SetUserData(const TGDPRKey& key, const TGDPRData& data) override;
    TString GetStorageTag() const override {
        return "gdpr-mock";
    }

private:
    TIntrusivePtr<IKvStorage<TGDPRKey, TGDPRData>> GDPRStorage;
};

class TGDPRDoubleStorage: public IGDPRStorage {
    using TStoragePtr = TIntrusivePtr<IGDPRStorage>;

public:
    TGDPRDoubleStorage(TStoragePtr oldStorage, TStoragePtr newStorage)
        : OldStorage(std::move(oldStorage)), NewStorage(std::move(newStorage))
    {
    }

    TAsyncGDPRGetUserDataResponse GetUserData(const TGDPRKey& key) override;
    TAsyncGDPRGetRequestsResponse GetRequests(int limit, int offset) override;
    TAsyncGDPRSetUserDataResponse SetUserData(const TGDPRKey& key, const TGDPRData& data) override;

    TString GetStorageTag() const override {
        return "gdpr-double";
    }

private:
    TIntrusivePtr<TGDPRDoubleStorage> IntrusiveThis() {
        return this;
    }

private:
    TStoragePtr OldStorage;
    TStoragePtr NewStorage;
};

TIntrusivePtr<IGDPRStorage> MakeGDPRStorage(const NCachalot::TYdbSettings& settings);
TIntrusivePtr<IGDPRStorage> MakeGDPRStorage(const NCachalot::TGDPRServiceSettings& settings);
} // namespace NCachalot

// For THashMap in TKvStorageMock.
template <>
struct THash<NCachalot::TGDPRKey> {
    size_t operator()(const NCachalot::TGDPRKey& object) const noexcept {
        return THash<TStringBuf>()(object.Puid);
    }
};
