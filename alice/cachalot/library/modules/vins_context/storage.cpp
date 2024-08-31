#include <alice/cachalot/library/modules/vins_context/storage.h>

#include <alice/cuttlefish/library/logging/dlog.h>
#include <library/cpp/digest/md5/md5.h>

#include <stdlib.h>
#include <util/string/printf.h>

using namespace NCachalot;

namespace {

    class TRequestError: public yexception {
    public:
        TRequestError(NCachalotProtocol::EResponseStatus responseStatus, const TString& message)
            : Status(responseStatus)
            , Message(message)
        {
        }

        const NCachalotProtocol::EResponseStatus Status;
        const TString Message;
    };

}  // anonymous namespace


TVinsContextKey::TVinsContextKey(const NCachalotProtocol::TVinsContextKey& key) {
    if (key.HasKey()) {
        Key = key.GetKey();
    }
    if (key.HasPuid()) {
        Puid = key.GetPuid();
    }
    Normalize();
    ComputeShardKey();
}

void TVinsContextKey::Normalize() {
}

void TVinsContextKey::ComputeShardKey() {
    TStringStream ss;
    ss << Key;
    TString md5LastHalf = MD5::Calc(ss.Str()).substr(16);
    ShardKey = std::strtoull(md5LastHalf.data(), nullptr, 16);
}

void TVinsContextKey::LoadFrom(const NCachalotProtocol::TVinsContextKey& key) {
    if (key.HasKey()) {
        Key = key.GetKey();
    } else {
        Key.clear();
    }
    if (key.HasPuid()) {
        Puid = key.GetPuid();
    } else {
        Puid.clear();
    }
    Normalize();
    ComputeShardKey();
}

void TVinsContextKey::SaveTo(NCachalotProtocol::TVinsContextKey& key) {
    key.SetKey(Key);
    key.SetPuid(Puid);
}


TVinsContextStorage::TVinsContextStorage(const NCachalot::TYdbSettings& settings)
    : Ydb{MakeIntrusive<TYdbContext>(settings)}
    , ReadTimeout{TDuration::Seconds(settings.ReadTimeoutSeconds())}
    , WriteTimeout{TDuration::Seconds(settings.WriteTimeoutSeconds())}
{
}

NThreading::TFuture<TVinsContextStorage::TEmptyResponse> TVinsContextStorage::Del(
    const TVinsContextKey& key, TChroniclerPtr /* chronicler */
) {
    DLOG("TVinsContextStorage::Del()");

    if (!Ydb->GetClient()) {
        DLOG("TVinsContextStorage::Del !Ydb->GetClient()");

        return NThreading::MakeFuture(
            TEmptyResponse()
                .SetKey(key)
                .SetError("YdbClient is null")
                .SetStatus(EResponseStatus::SERVICE_UNAVAILABLE)
        );
    }
    NThreading::TPromise<TEmptyResponse> response_promise = NThreading::NewPromise<TEmptyResponse>();
    NThreading::TPromise<NYdb::TStatus> status = NThreading::NewPromise<NYdb::TStatus>();

    static const TString QUERY_DELETE = Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $puid AS String;

        $to_delete = (
            SELECT * FROM data2 VIEW ix_data2_puid_async
            WHERE puid = $puid
            LIMIT 200
        );

        DELETE FROM data2 ON
        SELECT * FROM $to_delete;

        SELECT COUNT(*) FROM $to_delete;
    )", Ydb->GetSettings().Database().c_str());

    // sequence: ydbOperation -> onPreparedQueryResult -> onDataQueryResult
    int num_deleted_rows = -1;
    auto onDataQueryResult = [response_promise, status, key, &num_deleted_rows](const NYdb::NTable::TAsyncDataQueryResult& reply) mutable {
        TEmptyResponse response;
        const auto& result = reply.GetValueSync();

        if (result.IsSuccess()) {
            DLOG("TVinsContextStorage::Store ExecutingQuery OK");
            response = TEmptyResponse().SetStatus(EResponseStatus::CREATED);
            /*
            response_promise.SetValue(
                TEmptyResponse()
                    .SetStatus(EResponseStatus::CREATED)
            );
            */
        } else {
            TString issues = result.GetIssues().ToString();
            DLOG("TVinsContextStorage::Store ExecutingQuery FAILED: " << issues);
            response_promise.SetValue(
                TEmptyResponse()
                    .SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED)
                    .SetError(std::move(issues))
            );
            status.SetValue(result);
            return;
        }

        auto parser = result.GetResultSetParser(0);
        if (!parser.TryNextRow()) {
            throw TRequestError(EResponseStatus::NO_CONTENT, "Zero rows count");
        }
        try {
            size_t data = parser.ColumnParser(0).GetUint64();
            num_deleted_rows = data;
            response.ExtraData.NumDeletedRows = data;
        } catch (const std::exception& ex) {
            Cerr << "Ex: " << ex.what() << Endl;
        }
        status.SetValue(result);
        response_promise.SetValue(response);
    };

    auto onPreparedQueryResult = [response_promise, status, onDataQueryResult, key, writeTimeout = this->WriteTimeout](const NYdb::NTable::TAsyncPrepareQueryResult& reply) mutable {
        const auto& replyValue = reply.GetValueSync();

        if (!replyValue.IsSuccess()) {
            TString issues = replyValue.GetIssues().ToString();
            DLOG("TVinsContextStorage::Store PreparingQuery FAILED: " << issues);

            response_promise.SetValue(
                TEmptyResponse()
                    .SetStatus(EResponseStatus::QUERY_PREPARE_FAILED)
                    .SetError(std::move(issues))
            );
            status.SetValue(replyValue);
            return;
        }
        DLOG("TVinsContextStorage::Store PreparingQuery OK");
        NYdb::NTable::TDataQuery query = replyValue.GetQuery();

        DLOG("TVinsContextStorage::Del PreparingParams...");

        auto getParams = [&query, key]() {
            return query.GetParamsBuilder()
                .AddParam("$puid").String(key.Puid).Build()
                .Build();
        };

        DLOG("TVinsContextStorage::Store PreparingParams OK");
        DLOG("TVinsContextStorage::Store ExecutingQuery...");
        query.Execute(
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            std::move(getParams()),
            NYdb::NTable::TExecDataQuerySettings().ClientTimeout(writeTimeout)
        ).Subscribe(onDataQueryResult);
    };


    auto ydbOperation = [status, onPreparedQueryResult, key, writeTimeout = this->WriteTimeout](NYdb::NTable::TSession session) mutable {
        DLOG("TVinsContextStorage::Store Started");

        DLOG("TVinsContextStorage::Store PreparingQuery");
        NYdb::NTable::TAsyncPrepareQueryResult prepared = session.PrepareDataQuery(
            QUERY_DELETE,
            NYdb::NTable::TPrepareDataQuerySettings().ClientTimeout(writeTimeout)
        );
        prepared.Subscribe(onPreparedQueryResult);

        return status;
    };

    auto onYdbOperationFinish = [response_promise](const NYdb::TAsyncStatus& asyncStatus) mutable {
        const auto& status = asyncStatus.GetValueSync();
        if (!status.IsSuccess()) {
            response_promise.SetValue(
                TEmptyResponse()
                    .SetStatus(EResponseStatus::INTERNAL_ERROR)
                    .SetError(status.GetIssues().ToString())
            );
        }
    };

    Ydb->GetClient()->RetryOperation(ydbOperation, NYdb::NTable::TRetryOperationSettings().MaxRetries(1)).Subscribe(onYdbOperationFinish);
    return response_promise;
}

NThreading::TFuture<TVinsContextStorage::TMultipleRowsResponse> TVinsContextStorage::Get(
    const TVinsContextKey& /*key*/, TChroniclerPtr /* chronicler */
) {
    TVinsContextStorage::TMultipleRowsResponse response;
    return NThreading::MakeFuture(response);
}

NThreading::TFuture<TVinsContextStorage::TSingleRowResponse> TVinsContextStorage::GetSingleRow(
    const TVinsContextKey& /*key*/, TChroniclerPtr /* chronicler */
) {
    TVinsContextStorage::TSingleRowResponse response;
    return NThreading::MakeFuture(response);
}

NThreading::TFuture<TVinsContextStorage::TEmptyResponse> TVinsContextStorage::Set(
    const TVinsContextKey& /*key*/, const TString& /*data*/, int64_t /*ttl*/, TChroniclerPtr /* chronicler */
) {
    TVinsContextStorage::TEmptyResponse response;
    return NThreading::MakeFuture(response);
}
