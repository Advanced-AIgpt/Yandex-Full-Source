#include <alice/cachalot/library/modules/gdpr/storage.h>
#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/storage/ydb_operation.h>
#include <alice/cachalot/library/utils.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/digest/md5/md5.h>
#include <util/digest/city.h>
#include <util/string/printf.h>

#include <stdlib.h>

//using namespace NCachalot;

namespace NCachalot {

TVector<TString> Services = {"Memento", "YabioContext", "Datasync", "VinsContext", "Logs", "Notificator"};

class TGDPRYdbGetUserDataOperation:
    public TYdbOperationBase<TSingleRowResponse, TGDPRKey, TGDPRGetUserDataResponse> {
public:
    using TResponse = TSingleRowResponse;
    using TBase = TYdbOperationBase<TResponse, TGDPRKey, TGDPRGetUserDataResponse>;

    TGDPRYdbGetUserDataOperation()
        : TBase(&TMetrics::GetInstance().GDPRMetrics.YdbMetrics)
    {
        Cout << "Prefix: " << DatabaseName << Endl;
    }

protected:
    TString GetQueryTemplate() const override {
        return Sprintf(R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");
            DECLARE $shard_key AS Uint64;
            DECLARE $puid AS String;
            SELECT
                MementoStatus,
                MementoTs,
                YabioContextStatus,
                YabioContextTs,
                VinsContextStatus,
                VinsContextTs,
                DatasyncStatus,
                DatasyncTs,
                LogsStatus,
                LogsTs,
                NotificatorStatus,
                NotificatorTs
            FROM gdpr_statuses
            WHERE
                shard_key == $shard_key
                AND
                puid == $puid
            LIMIT 1
            ;
        )", DatabaseName.c_str());
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return paramsBuilder
            .AddParam("$puid").String(Key.Puid).Build()
            .AddParam("$shard_key").Uint64(Key.ShardKey).Build()
            .Build();
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        // NThreading::TPromise<TResponse>* responsePromise,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        //TResponse response;
        TResponse& response = LastResponse;
        response.SetKey(Key).SetStats(Stats);

        if (queryResult.IsSuccess()) {
            if (queryResult.GetResultSets().size() != 1) {
                response.SetStatus(EResponseStatus::INTERNAL_ERROR);
                Metrics->OnExecuteError(StartTime);
            } else {
                NYdb::TResultSetParser parser = queryResult.GetResultSetParser(0);
                if (parser.TryNextRow()) {
                    TGDPRGetUserDataResponse data;
                    for (const TString& service : Services) {
                        TServiceStatus status;
                        status.Service = service;
                        status.Status = parser.ColumnParser(service + "Status").GetOptionalString().GetOrElse("Unknown");
                        status.Timestamp = parser.ColumnParser(service + "Ts").GetOptionalString().GetOrElse("Unknown");
                        data.Data.Statuses.push_back(status);
                    }
                    data.Puid = Key.Puid;
                    response.SetData(std::move(data));
                    response.SetStatus(EResponseStatus::OK);
                    Metrics->OnOk(StartTime);
                } else {
                    DLOG("TGDPRYdbGetUserDataOperation: NOT FOUND");
                    Metrics->OnNotFound(StartTime);
                    response.SetStatus(EResponseStatus::NOT_FOUND);
                    response.SetError("Puid not found");
                }
            }
        } else {
            response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            response.SetError(queryResult.GetIssues().ToString());
            Metrics->OnExecuteError(StartTime);
        }
        // responsePromise->SetValue(response);
        status->SetValue(queryResult);
    }
};

class TGDPRYdbGetRequestsOperation:
    public TYdbOperationBase<TMultipleRowsResponse, TGDPRKey, TGDPRGetRequestsResponse> {
public:
    using TResponse = TMultipleRowsResponse;
    using TBase = TYdbOperationBase<TResponse, TGDPRKey, TGDPRGetRequestsResponse>;

    TGDPRYdbGetRequestsOperation(int limit, int offset)
        : TBase(&TMetrics::GetInstance().GDPRMetrics.YdbMetrics)
        , Limit(limit)
        , Offset(offset)
    {
    }

protected:
    TString GetQueryTemplate() const override {
        return Sprintf(R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");
            DECLARE $limit AS Uint64;
            DECLARE $offset AS Uint64;
            $requested = ["requested", "failed"];

            SELECT
                puid,
                MementoStatus,
                MementoTs,
                YabioContextStatus,
                YabioContextTs,
                VinsContextStatus,
                VinsContextTs,
                DatasyncStatus,
                DatasyncTs,
                LogsStatus,
                LogsTs,
                NotificatorStatus,
                NotificatorTs,
                MAX_OF(
                    COALESCE(MementoTs, "0"),
                    COALESCE(YabioContextTs, "0"),
                    COALESCE(VinsContextTs, "0"),
                    COALESCE(DatasyncTs, "0"),
                    COALESCE(LogsTs, "0"),
                    COALESCE(NotificatorTs, "0")
                    ) AS LatestTS
            FROM gdpr_statuses
            WHERE
                MementoStatus IN $requested
                OR
                YabioContextStatus IN $requested
                OR
                VinsContextStatus IN $requested
                OR
                DatasyncStatus IN $requested
                OR
                LogsStatus IN $requested
                OR
                NotificatorStatus IN $requested
            ORDER BY
                LatestTS
            LIMIT
                $limit
            OFFSET
                $offset
            ;
        )", DatabaseName.c_str());
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return paramsBuilder
            .AddParam("$limit").Uint64(Limit).Build()
            .AddParam("$offset").Uint64(Offset).Build()
            .Build();
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        TResponse& response = LastResponse;
        response.SetKey(Key).SetStats(Stats);

        if (queryResult.IsSuccess()) {
            TGDPRGetRequestsResponse result;
            NYdb::TResultSetParser parser = queryResult.GetResultSetParser(0);
            while (parser.TryNextRow()) {
                TGDPRData data;
                for (const TString& service : Services) {
                    TServiceStatus status;
                    status.Service = service;
                    status.Status = parser.ColumnParser(service + "Status").GetOptionalString().GetOrElse("Unknown");
                    status.Timestamp = parser.ColumnParser(service + "Ts").GetOptionalString().GetOrElse("Unknown");
                    data.Statuses.push_back(status);
                }
                auto puid =  parser.ColumnParser("puid").GetOptionalString().GetOrElse("Unknown");
                result.PersonalizedData[puid] = data;
                Metrics->OnOk(StartTime);
            }
            result.Limit = Limit;
            result.Offset = Offset;
            response.SetData(std::move(result));
            response.SetStatus(EResponseStatus::OK);
        } else {
            DLOG("TGDPRYdbGetRequestsOperation Bad req: " << queryResult.GetIssues().ToString());
            response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            response.SetError(queryResult.GetIssues().ToString());
            Metrics->OnExecuteError(StartTime);
        }
        status->SetValue(queryResult);
    }
private:
    uint64_t Limit = 0;
    uint64_t Offset = 0;
};

class TGDPRYdbSetUserDataOperation:
    public TYdbOperationBase<TSingleRowSetResponse, TGDPRKey, TGDPRSetUserDataResponse> {
public:
    using TResponse = TSingleRowSetResponse;
    using TBase = TYdbOperationBase<TResponse, TGDPRKey, TGDPRSetUserDataResponse>;

    TGDPRYdbSetUserDataOperation(TString serviceName, TGDPRData gdprData)
        : TBase(&TMetrics::GetInstance().GDPRMetrics.YdbMetrics)
        , ServiceName(serviceName)
        , GDPRData(gdprData)
    {
    }

protected:
    TString GetQueryTemplate() const override {
        return Sprintf(R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");
            DECLARE $shard_key AS Uint64;
            DECLARE $puid AS String;
            DECLARE $status AS String;
            DECLARE $ts AS String;

            UPSERT INTO gdpr_statuses
            (shard_key, puid, %sStatus, %sTs)
            VALUES
            ($shard_key, $puid, $status, $ts);
        )", DatabaseName.c_str(), ServiceName.c_str(), ServiceName.c_str());
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        TString ts = ToString(StartTime);
        if (GDPRData.Statuses[0].Timestamp) {
            ts = GDPRData.Statuses[0].Timestamp;
        }
        return paramsBuilder
            .AddParam("$puid").String(Key.Puid).Build()
            .AddParam("$shard_key").Uint64(Key.ShardKey).Build()
            .AddParam("$status").String(GDPRData.Statuses[0].Status).Build()
            .AddParam("$ts").String(ts).Build()
            .Build();
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult
    ) override {
        TResponse& response = LastResponse;
        response.SetKey(Key).SetStats(Stats);

        if (queryResult.IsSuccess()) {
            response.SetStatus(EResponseStatus::OK);
            Metrics->OnOk(StartTime);
        } else {
            response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            response.SetError(queryResult.GetIssues().ToString());
            Metrics->OnExecuteError(StartTime);
        }
        status->SetValue(queryResult);
    }
private:
    TString ServiceName;
    TGDPRData GDPRData;
};


void TGDPRKey::ComputeShardKey() {
    ShardKey = CityHash64(Puid);
}

TGDPRYdbStorage::TGDPRYdbStorage(const NCachalot::TYdbSettings& settings)
    : Ydb(settings)
{
}

TAsyncGDPRGetUserDataResponse TGDPRYdbStorage::GetUserData(const TGDPRKey& key) {
    auto operation = MakeIntrusive<TGDPRYdbGetUserDataOperation>();
    operation->SetKey(key);
    const auto operationTimeout = TDuration::MilliSeconds(1000);
    operation->MutableOperationSettings()
        .MaxRetries(2)
        .OperationTimeout(operationTimeout)
        .ClientTimeout(operationTimeout + TDuration::MilliSeconds(100));
    operation->SetDatabase(Ydb.GetSettings().Database());
    return operation->Execute(Ydb.GetClient());
}

TAsyncGDPRGetRequestsResponse TGDPRYdbStorage::GetRequests(int limit, int offset) {
    auto operation = MakeIntrusive<TGDPRYdbGetRequestsOperation>(limit, offset);
    const auto operationTimeout = TDuration::MilliSeconds(1000);
    operation->MutableOperationSettings()
        .MaxRetries(2)
        .OperationTimeout(operationTimeout)
        .ClientTimeout(operationTimeout + TDuration::MilliSeconds(100));
    operation->SetDatabase(Ydb.GetSettings().Database());
    auto res = operation->Execute(Ydb.GetClient());
    return res;
}

TAsyncGDPRSetUserDataResponse TGDPRYdbStorage::SetUserData(const TGDPRKey& key, const TGDPRData& data) {
    auto operation = MakeIntrusive<TGDPRYdbSetUserDataOperation>(data.Statuses[0].Service, data);
    operation->SetKey(key);
    const auto operationTimeout = TDuration::MilliSeconds(1000);
    operation->MutableOperationSettings()
        .MaxRetries(2)
        .OperationTimeout(operationTimeout)
        .ClientTimeout(operationTimeout + TDuration::MilliSeconds(100));
    operation->SetDatabase(Ydb.GetSettings().Database());
    return operation->Execute(Ydb.GetClient());
}

TGDPRStorageMock::TGDPRStorageMock()
    : GDPRStorage(MakeIntrusive<TKvStorageMock<TGDPRKey, TGDPRData>>())
{
}

TAsyncGDPRGetUserDataResponse TGDPRStorageMock::GetUserData(const TGDPRKey& /*key*/) {
    return NThreading::MakeFuture(TSingleRowResponse());
}

TAsyncGDPRGetRequestsResponse TGDPRStorageMock::GetRequests(int /*limit*/, int /*offset*/) {
    return NThreading::MakeFuture(TMultipleRowsResponse());
}

TAsyncGDPRSetUserDataResponse TGDPRStorageMock::SetUserData(const TGDPRKey& /*key*/, const TGDPRData& /*data*/) {
    return NThreading::MakeFuture(TSingleRowSetResponse());
}

TAsyncGDPRGetUserDataResponse TGDPRDoubleStorage::GetUserData(const TGDPRKey& key) {
    return OldStorage->GetUserData(key);
}

TAsyncGDPRGetRequestsResponse TGDPRDoubleStorage::GetRequests(int limit, int offset) {
    return OldStorage->GetRequests(limit, offset);
}

TAsyncGDPRSetUserDataResponse TGDPRDoubleStorage::SetUserData(const TGDPRKey& key, const TGDPRData& data) {
    // TPromiseGDPRSetUserDataResponse response;
    auto response = NThreading::NewPromise<TSingleRowSetResponse>();
    auto mngr = NThreading::TSubscriptionManager::NewInstance();

    auto This = IntrusiveThis();
    mngr->Subscribe(
        WaitBoth(
            OldStorage->SetUserData(key, data),
            NewStorage->SetUserData(key, data)
        ),
        [key, response, This](const auto& future) mutable {
            const auto& [first, second] = future.GetValueSync();

            TSingleRowSetResponse result;
            result.SetKey(key);
            result.SetStatus(first.Status);
            response.SetValue(result);
        }
    );

    return response;
}

TIntrusivePtr<IGDPRStorage> MakeGDPRStorage(const NCachalot::TYdbSettings& settings) {
    if (settings.IsFake()) {
        return MakeIntrusive<TGDPRStorageMock>();
    }
    return MakeIntrusive<TGDPRYdbStorage>(settings);
}

TIntrusivePtr<IGDPRStorage> MakeGDPRStorage(const NCachalot::TGDPRServiceSettings& settings) {
    auto oldStorage = MakeGDPRStorage(settings.OldYdb());
    auto newStorage = MakeGDPRStorage(settings.NewYdb());
    return MakeIntrusive<TGDPRDoubleStorage>(oldStorage, newStorage);
}

}   // namespace NCachalot
