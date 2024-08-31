#pragma once

#include <alice/cachalot/library/storage/ydb.h>
#include <alice/cachalot/library/storage/storage.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <util/digest/city.h>
#include <util/digest/multi.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>

namespace NCachalot {

struct TTakeoutResultsKey {
    TTakeoutResultsKey() = default;
    void ComputeShardKey();

    bool operator==(const TTakeoutResultsKey& other) const {
        return JobId == other.JobId;
    }

    static ui64 ComputeShardKey(TString jobId) {
        return CityHash64(jobId);
    }

    ui64 ShardKey = 0;
    TString JobId;
};

struct TTakeoutResult {
    TString JobId;
    TString Puid;
    TVector<TString> Texts;
};

struct TTakeoutResults {
    TVector<TTakeoutResult> Results;
};

struct TTakeoutSetResultsResponse {
};


using TTakeoutGetResultsResponse = TTakeoutResult;
using TSingleRowTakeoutSetResponse = TSingleRowStorageResponse<TTakeoutResultsKey, TTakeoutSetResultsResponse>;
using TSingleRowTakeoutGetResponse = TSingleRowStorageResponse<TTakeoutResultsKey, TTakeoutResult>;
using TAsyncTakeoutGetResultsResponse = NThreading::TFuture<TSingleRowTakeoutGetResponse>;
using TAsyncTakeoutSetResultsResponse = NThreading::TFuture<TSingleRowTakeoutSetResponse>;


class ITakeoutResultsStorage: public TThrRefBase {
public:
    virtual TAsyncTakeoutSetResultsResponse SetResults(const TTakeoutResults&) = 0;
    virtual TAsyncTakeoutGetResultsResponse GetResults(const TTakeoutResultsKey& key, uint64_t limit, uint64_t offset) = 0;
    virtual TString GetStorageTag() const = 0;
};

class TTakeoutResultsYdbStorage: public ITakeoutResultsStorage {
public:
    TTakeoutResultsYdbStorage(const NCachalot::TYdbSettings& settings);

    TAsyncTakeoutSetResultsResponse SetResults(const TTakeoutResults&) override;
    TAsyncTakeoutGetResultsResponse GetResults(const TTakeoutResultsKey& key, uint64_t limit, uint64_t offset) override;
    TString GetStorageTag() const override {
        return "takeout-results-ydb";
    }
private:
    TYdbContext Ydb;
};

TIntrusivePtr<ITakeoutResultsStorage> MakeTakeoutResultsStorage(const NCachalot::TYdbSettings& settings);

} // namespace NCachalot

// For THashMap in TKvStorageMock.
template <>
struct THash<NCachalot::TTakeoutResultsKey> {
    size_t operator()(const NCachalot::TTakeoutResultsKey& object) const noexcept {
        return THash<TStringBuf>()(object.JobId);
    }
};
