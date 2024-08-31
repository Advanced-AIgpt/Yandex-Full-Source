#include <alice/cachalot/library/modules/cache/storage.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/inmemory/imdb.h>
#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/storage/ydb_operation.h>

#include <util/digest/city.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/string/printf.h>


namespace NCachalot {


class TSimpleSetOperation : public TYdbSetOperationBase<TString, TString> {
public:
    using TResponse = TSimpleEmptyResponse;
    using TBase = TYdbSetOperationBase<TString, TString>;

    explicit TSimpleSetOperation(TCacheYdbOperationSettings settings)
        : TBase(&(TMetrics::GetInstance().CacheServiceMetrics.YdbSet))
        , Settings(std::move(settings))
    {
    }

protected:
    TString GetQueryTemplate() const override;
    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override;

private:
    TCacheYdbOperationSettings Settings;
};


class TSimpleGetOperation : public TYdbOperationBase<TSimpleSingleRowResponse, TString, TString> {
public:
    using TResponse = TSimpleSingleRowResponse;
    using TBase = TYdbOperationBase<TResponse, TString, TString>;

    explicit TSimpleGetOperation(TCacheYdbOperationSettings settings)
        : TBase(&(TMetrics::GetInstance().CacheServiceMetrics.YdbGet))
        , Settings(std::move(settings))
    {
    }

protected:
    TString GetQueryTemplate() const override;
    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override;
    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult) override;

    NYdb::NTable::TTxSettings GetTransactionMode() const override {
        return NYdb::NTable::TTxSettings::StaleRO();
    }

private:
    TCacheYdbOperationSettings Settings;
};


class TSimpleDeleteOperation : public TYdbDeleteOperationBase<TString, TString> {
public:
    using TResponse = TSimpleEmptyResponse;
    using TBase = TYdbDeleteOperationBase<TString, TString>;

    explicit TSimpleDeleteOperation(TCacheYdbOperationSettings settings)
        : TBase(&(TMetrics::GetInstance().CacheServiceMetrics.YdbDel))
        , Settings(std::move(settings))
    {
    }

protected:
    TString GetQueryTemplate() const override;
    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override;

private:
    TCacheYdbOperationSettings Settings;
};


TString TSimpleSetOperation::GetQueryTemplate() const {
    return Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $shard AS Uint64;
        DECLARE $key AS Utf8;
        DECLARE $ttl AS Timestamp;
        DECLARE $data AS String;
        UPSERT INTO %s (ShardId, Key, Deadline, %s) VALUES
            ($shard, $key, $ttl, $data)
    )", DatabaseName.c_str(), TableName.c_str(), Settings.DataColumnName().c_str());
}

NYdb::TParams TSimpleSetOperation::BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const {
    Y_ENSURE(TtlSeconds > 0, "TTL must be positive");

    return paramsBuilder
        .AddParam("$shard").Uint64(CityHash64(Key.data(), Key.size())).Build()
        .AddParam("$key").Utf8(Key).Build()
        .AddParam("$ttl").Timestamp(TDuration::Seconds(TtlSeconds).ToDeadLine()).Build()
        .AddParam("$data").String(Data).Build()
        .Build();
}


TString TSimpleGetOperation::GetQueryTemplate() const {
    return Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $shard AS Uint64;
        DECLARE $key AS Utf8;
        SELECT %s FROM %s WHERE ShardId == $shard AND Key == $key
        LIMIT 1;
    )", DatabaseName.c_str(), Settings.DataColumnName().c_str(), TableName.c_str());
}

NYdb::TParams TSimpleGetOperation::BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const {
    return paramsBuilder
        .AddParam("$shard").Uint64(CityHash64(Key.data(), Key.size())).Build()
        .AddParam("$key").Utf8(Key).Build()
        .Build();
}

TString TSimpleDeleteOperation::GetQueryTemplate() const {
    return Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $shard AS Uint64;
        DECLARE $key AS Utf8;
        DELETE FROM %s WHERE ShardId == $shard AND Key == $key
    )", DatabaseName.c_str(), TableName.c_str());
}

NYdb::TParams TSimpleDeleteOperation::BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const {
    return paramsBuilder
        .AddParam("$shard").Uint64(CityHash64(Key.data(), Key.size())).Build()
        .AddParam("$key").Utf8(Key).Build()
        .Build();
}

void TSimpleGetOperation::ProcessQueryResult(
    NThreading::TPromise<NYdb::TStatus>* status,
    const NYdb::NTable::TDataQueryResult& queryResult
) {
    LastResponse.SetKey(Key);
    LastResponse.SetStats(Stats);

    if (queryResult.IsSuccess()) {
        if (queryResult.GetResultSets().size() != 1) {
            LastResponse.SetStatus(EResponseStatus::NO_CONTENT);
            Metrics->OnNotFound(StartTime);
        } else {
            NYdb::TResultSetParser parser = queryResult.GetResultSetParser(0);
            if (parser.TryNextRow()) {
                LastResponse.SetData(
                    parser.ColumnParser(Settings.DataColumnName()).GetOptionalString().GetOrElse("")
                );
                LastResponse.SetStatus(EResponseStatus::OK);
                Metrics->OnOk(StartTime);
            } else {
                LastResponse.SetStatus(EResponseStatus::NO_CONTENT);
                Metrics->OnNotFound(StartTime);
            }
        }
    } else {
        LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED).SetError(queryResult.GetIssues().ToString());
        Metrics->OnExecuteError(StartTime);
    }
    status->SetValue(queryResult);
}


TYdbStorage::TYdbStorage(const NCachalot::TYdbSettings& settings, TCacheYdbOperationSettings operationSettings)
    : Ydb(settings)
    , OperationSettings(std::move(operationSettings))
{ }


TYdbStorage::~TYdbStorage()
{ }


NThreading::TFuture<TYdbStorage::TEmptyResponse> TYdbStorage::Set(
    const TString& key, const TString& data, int64_t ttl, TChroniclerPtr chronicler
) {
    if (data.size() > Config.MaxDataSize) {
        TEmptyResponse response;
        response.SetKey(key);
        response.SetError(TStringBuilder() << "Record size (" << data.size() << ") exceeds the limit.");
        response.SetStatus(EResponseStatus::BAD_REQUEST);
        return NThreading::MakeFuture(response);
    }

    if (ttl <= 0) {
        ttl = Ydb.GetSettings().TimeToLiveSeconds();
    }

    int64_t maxTTL = static_cast<int64_t>(Ydb.GetSettings().MaxTimeToLiveSeconds());
    if (ttl > maxTTL) {
        ttl = maxTTL;
    }
    
    TIntrusivePtr<TSimpleSetOperation> operation = MakeIntrusive<TSimpleSetOperation>(OperationSettings);
    operation->SetTtl(ttl);
    operation->SetKey(key);
    operation->SetData(data);
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetTable(Ydb.GetSettings().Table());
    operation->SetLogger(chronicler);
    return operation->Execute(Ydb.GetClient());
}


NThreading::TFuture<TYdbStorage::TSingleRowResponse> TYdbStorage::GetSingleRow(
    const TString& key, TChroniclerPtr chronicler
) {
    TIntrusivePtr<TSimpleGetOperation> operation = MakeIntrusive<TSimpleGetOperation>(OperationSettings);
    operation->SetKey(key);
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetTable(Ydb.GetSettings().Table());
    operation->SetLogger(chronicler);
    return operation->Execute(Ydb.GetClient());
}


NThreading::TFuture<TYdbStorage::TMultipleRowsResponse> TYdbStorage::Get(
    const TString& key, TChroniclerPtr
) {
    TMultipleRowsResponse response;
    response.SetKey(key);
    response.SetError("IStorage::Get(MultipleRows) method is not implemeted for SimpleYdbStorage");
    response.SetStatus(EResponseStatus::NOT_IMPLEMENTED);
    return NThreading::MakeFuture(response);
}


NThreading::TFuture<TYdbStorage::TEmptyResponse> TYdbStorage::Del(
    const TString& key, TChroniclerPtr chronicler
) {
    TIntrusivePtr<TSimpleDeleteOperation> operation = MakeIntrusive<TSimpleDeleteOperation>(OperationSettings);
    operation->SetKey(key);
    operation->SetDatabase(Ydb.GetSettings().Database());
    operation->SetTable(Ydb.GetSettings().Table());
    operation->SetLogger(chronicler);
    return operation->Execute(Ydb.GetClient());
}


TIntrusivePtr<TSimpleKvStorage> MakeSimpleYdbStorage(
    const NCachalot::TYdbSettings& settings,
    TCacheYdbOperationSettings operationSettings
) {
    auto storage = MakeStorage<TYdbStorage>(settings.IsFake(), settings, std::move(operationSettings));
    storage->MutableConfig()->MaxDataSize = settings.MaxDataSize();
    storage->MutableConfig()->DefaultTtlSeconds = 60 * 60;
    return storage;
}


TIntrusivePtr<TSimpleKvStorage> MakeSimpleImdbStorage(
    const NCachalot::TInmemoryStorageSettings& settings,
    TInmemoryStorageMetrics* metrics) {
    return MakeInmemoryStorage<TSimpleKvStorage::TKey, TSimpleKvStorage::TData>(settings, metrics);
}


}   // namespace NCachalot
