#include <alice/cachalot/library/modules/takeout/storage.h>

#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/storage/ydb_operation.h>

#include <alice/cuttlefish/library/logging/dlog.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/generic/guid.h>
#include <util/string/printf.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>

#include <stdlib.h>

//using namespace NCachalot;

namespace NCachalot {

class TTakeoutYdbGetResultsOperation:
    public TYdbOperationBase<TSingleRowTakeoutGetResponse, TTakeoutResultsKey, TTakeoutGetResultsResponse> {
public:
    using TResponse = TSingleRowTakeoutGetResponse;
    using TBase = TYdbOperationBase<TResponse, TTakeoutResultsKey, TTakeoutGetResultsResponse>;

    TTakeoutYdbGetResultsOperation(uint64_t limit, uint64_t offset)
        : TBase(&TMetrics::GetInstance().TakeoutMetrics.YdbMetrics)
        , Limit(limit)
        , Offset(offset)
    {
    }

protected:
    TString GetQueryTemplate() const override {
        return Sprintf(R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");
            DECLARE $shard_key AS Uint64;
            DECLARE $limit AS Uint64;
            DECLARE $offset AS Uint64;
            DECLARE $job_id AS String;
            SELECT
                job_id,
                chunk_id,
                puid,
                created_at,
                texts
            FROM takeout_results
            WHERE
                shard_key == $shard_key
                AND
                job_id == $job_id
            ORDER BY
                chunk_id
            LIMIT
                $limit
            OFFSET
                $offset
            ;
        )", DatabaseName.c_str());
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return paramsBuilder
            .AddParam("$shard_key").Uint64(Key.ShardKey).Build()
            .AddParam("$job_id").String(Key.JobId).Build()
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
            if (queryResult.GetResultSets().size() != 1) {
                response.SetStatus(EResponseStatus::INTERNAL_ERROR);
                Metrics->OnExecuteError(StartTime);
            } else {
                NYdb::TResultSetParser parser = queryResult.GetResultSetParser(0);
                TTakeoutResult data;
                data.Puid = "undefined";
                while (parser.TryNextRow()) {
                    data.JobId = parser.ColumnParser("job_id").GetOptionalString().GetOrElse("Unknown");
                    data.Puid = parser.ColumnParser("puid").GetOptionalString().GetOrElse("Unknown");
                    TString textsJsonString = parser.ColumnParser("texts").GetOptionalJson().GetOrElse("{\"texts\":[]}");

                    NJson::TJsonValue textsJson;
                    try {
                        NJson::ReadJsonTree(textsJsonString, &textsJson, /* throwOnError = */ true);
                        for (const auto& text : textsJson["texts"].GetArraySafe()) {
                            data.Texts.push_back(text.GetString());
                        }
                    } catch (const std::exception& ex) {
                        response.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
                        response.SetError("Bad Json from the base");
                        Metrics->OnExecuteError(StartTime);
                        DLOG(TStringBuilder{} << "Exception: " << ex.what() << "; Bad Json from the base: " << textsJsonString);
                        return;
                    }
                }

                response.SetData(std::move(data));
                response.SetStatus(EResponseStatus::OK);
                Metrics->OnOk(StartTime);
            }
        } else {
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


static constexpr size_t MAX_CHUNK_SIZE = 1000;

class TTakeoutYdbSetResultsOperation:
    public TYdbOperationBase<TSingleRowTakeoutSetResponse, TTakeoutResultsKey, TTakeoutSetResultsResponse> {
public:
    using TResponse = TSingleRowTakeoutSetResponse;
    using TBase = TYdbOperationBase<TResponse, TTakeoutResultsKey, TTakeoutSetResultsResponse>;

    TTakeoutYdbSetResultsOperation(TTakeoutResults results)
        : TBase(&TMetrics::GetInstance().TakeoutMetrics.YdbMetrics)
        , Results(results)
    {
    }

protected:
    void AddLineToQuery(NYdb::TParamValueBuilder& param, const TTakeoutResult& result, NJson::TJsonValue& currentChunk, const TInstant& createdAt) const {
        TString chunkId = CreateGuidAsString();
        TString data = WriteJson(currentChunk, false);

        param.AddListItem()
            .BeginStruct()
            .AddMember("job_id").String(result.JobId)
            .AddMember("chunk_id").String(chunkId)
            .AddMember("shard_key").Uint64(TTakeoutResultsKey::ComputeShardKey(result.JobId))
            .AddMember("created_at").Timestamp(createdAt)
            .AddMember("puid").String(result.Puid)
            .AddMember("texts").Json(data)
            .EndStruct();
    }

    TString GetQueryTemplate() const override {
        return Sprintf(R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");

            DECLARE $texts AS List<
                Struct<
                    job_id: String,
                    chunk_id: String,
                    shard_key: Uint64,
                    created_at: Timestamp,
                    puid: String,
                    texts: Json
                >
            >;

            UPSERT INTO takeout_results (
                SELECT
                    job_id,
                    chunk_id,
                    shard_key,
                    created_at,
                    puid,
                    texts
                FROM AS_TABLE($texts)
            );
        )", DatabaseName.c_str());
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        auto& param = paramsBuilder.AddParam("$texts");
        if (Results.Results.empty()) {
            param.EmptyList(
                NYdb::TTypeBuilder()
                    .BeginStruct()
                        .AddMember("job_id").Primitive(NYdb::EPrimitiveType::String)
                        .AddMember("chunk_id").Primitive(NYdb::EPrimitiveType::String)
                        .AddMember("shard_key").Primitive(NYdb::EPrimitiveType::Uint64)
                        .AddMember("created_at").Primitive(NYdb::EPrimitiveType::Timestamp)
                        .AddMember("puid").Primitive(NYdb::EPrimitiveType::String)
                        .AddMember("texts").Primitive(NYdb::EPrimitiveType::Json)
                    .EndStruct()
                .Build()
            );
            return paramsBuilder.Build();
        }
        param.BeginList();

        for (const auto& result : Results.Results) {
            NJson::TJsonValue currentChunk;
            currentChunk["texts"] = NJson::TJsonArray();
            size_t currentChunkLength = 0;
            for (const auto& text : result.Texts) {
                if (currentChunkLength + text.size() > MAX_CHUNK_SIZE) {
                    AddLineToQuery(param, result, currentChunk, StartTime);

                    currentChunk["texts"] = NJson::TJsonArray();
                    currentChunk["texts"].AppendValue(text);
                    currentChunkLength = text.size();
                    continue;
                }

                currentChunk["texts"].AppendValue(text);
                currentChunkLength += text.size();
            }

            AddLineToQuery(param, result, currentChunk, StartTime);
        }
        param.EndList().Build();

        return paramsBuilder.Build();
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
    TTakeoutResults Results;
};

void TTakeoutResultsKey::ComputeShardKey() {
    ShardKey = TTakeoutResultsKey::ComputeShardKey(JobId);
}

TTakeoutResultsYdbStorage::TTakeoutResultsYdbStorage(const NCachalot::TYdbSettings& settings)
    : Ydb(settings)
{
}

TAsyncTakeoutGetResultsResponse TTakeoutResultsYdbStorage::GetResults(const TTakeoutResultsKey& key, uint64_t limit, uint64_t offset) {
    auto operation = MakeIntrusive<TTakeoutYdbGetResultsOperation>(limit, offset);
    operation->SetKey(key);
    const auto operationTimeout = TDuration::MilliSeconds(1000);
    operation->MutableOperationSettings()
        .MaxRetries(2)
        .OperationTimeout(operationTimeout)
        .ClientTimeout(operationTimeout + TDuration::MilliSeconds(100));
    operation->SetDatabase(Ydb.GetSettings().Database());
    auto client = Ydb.GetClient();
    return operation->Execute(client);
}

TAsyncTakeoutSetResultsResponse TTakeoutResultsYdbStorage::SetResults(const TTakeoutResults& results) {
    auto operation = MakeIntrusive<TTakeoutYdbSetResultsOperation>(results);
    const auto operationTimeout = TDuration::MilliSeconds(1000);
    operation->MutableOperationSettings()
        .MaxRetries(2)
        .OperationTimeout(operationTimeout)
        .ClientTimeout(operationTimeout + TDuration::MilliSeconds(100));
    operation->SetDatabase(Ydb.GetSettings().Database());
    return operation->Execute(Ydb.GetClient());
}

TIntrusivePtr<ITakeoutResultsStorage> MakeTakeoutResultsStorage(const NCachalot::TYdbSettings& settings) {
    return MakeIntrusive<TTakeoutResultsYdbStorage>(settings);
}

}   // namespace NCachalot
