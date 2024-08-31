#include <alice/cachalot/library/modules/megamind_session/storage.h>
#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/utils.h>
#include <alice/cachalot/library/storage/ydb_operation.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/blockcodecs/core/codecs.h>

#include <util/generic/maybe.h>


namespace NCachalot {

namespace {

TString CropKeyRequestId(const TString& key) {
    auto delim = key.find_last_of('@');
    if (delim == TString::npos) {
        return key;
    } else {
        return key.substr(0, delim);
    }
}

} // anonymous namespace


bool TMegamindSessionKey::operator==(const TMegamindSessionKey& rhs) const {
    return ShardKey == rhs.ShardKey && Key == rhs.Key;
}

uint64_t TMegamindSessionKey::GetMemoryUsage() const {
    return sizeof(*this) + Key.capacity();
}


uint64_t TMegamindSessionData::GetMemoryUsage() const {
    return sizeof(*this) + Puid.capacity() + Data.capacity();
}

TString TMegamindSessionData::ToString() const {
    TString result;
    TStringOutput out(result);
    this->Save(&out);
    return result;
}

TMegamindSessionData TMegamindSessionData::FromString(const TString& buffer) {
    TMegamindSessionData result;
    TStringInput input(buffer);
    result.Load(&input);
    return result;
}


class TMegamingSessionLoadYdbOperation :
    public TYdbOperationBase<TMegamindSessionYdbStorage::TSingleRowResponse, TMegamindSessionKey, TMegamindSessionData> {
public:
    using TResponse = TMegamindSessionYdbStorage::TSingleRowResponse;
    using TBase = TYdbOperationBase<TResponse, TMegamindSessionKey, TMegamindSessionData>;

    TMegamingSessionLoadYdbOperation()
        : TBase(&TMetrics::GetInstance().MegamindSessionMetrics.YdbGetMetrics)
    {
    }

protected:
    TString GetQueryTemplate() const override {
        static const TString QUERY_LOAD = R"(
            DECLARE $shard_key AS Uint64;
            DECLARE $key AS String;

            SELECT
                data,
                puid
            FROM
                data2
            WHERE
                shard_key = $shard_key
                AND
                key = $key
            LIMIT 1;
        )";
        return QUERY_LOAD;
    }

    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& result
    ) override {
        Log<NEvClass::MMYdbGetProcessing>();

        LastResponse.SetKey(Key);
        LastResponse.SetStats(Stats);
        LastResponse.SetStatus(EResponseStatus::OK);

        if (!result.IsSuccess()) {
            Log<NEvClass::MMYdbGetErorr>();
            LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED);
            LastResponse.SetError(result.GetIssues().ToString());
        } else if (result.GetResultSets().empty()) {
            Log<NEvClass::MMYdbGetNotFound>();
            LastResponse.SetStatus(EResponseStatus::NO_CONTENT);
            LastResponse.SetError("Empty result set");
        } else {
            auto parser = result.GetResultSetParser(0);

            DLOG("TMegamindSessionYdbStorage::Load RetryOperation ParsingResultSet");
            if (!parser.TryNextRow()) {
                Log<NEvClass::MMYdbGetNotFound>();
                LastResponse.SetStatus(EResponseStatus::NO_CONTENT);
                LastResponse.SetError("Zero rows count");
            } else {
                Log<NEvClass::MMYdbGetFound>();
                TMegamindSessionData data;
                data.Data = parser.ColumnParser("data").GetOptionalString().GetOrElse("");
                data.Puid = parser.ColumnParser("puid").GetOptionalString().GetOrElse("");

                if (!data.Data) {
                    LastResponse.SetStatus(EResponseStatus::NO_CONTENT);
                    LastResponse.SetError("Empty data column");
                } else {
                    LastResponse.SetData(data);
                }
            }
        }

        status->SetValue(result);
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return paramsBuilder
            .AddParam("$shard_key")
                .Uint64(Key.ShardKey)
                .Build()
            .AddParam("$key")
                .String(Key.Key)
                .Build()
            .Build();
    }

    NYdb::NTable::TTxSettings GetTransactionMode() const override {
        return NYdb::NTable::TTxSettings::OnlineRO(NYdb::NTable::TTxOnlineSettings().AllowInconsistentReads(false));
    }
};


class TMegamingSessionStoreYdbOperationBase :
    public TYdbSetOperationBase<TMegamindSessionKey, TMegamindSessionData> {
public:
    using TBase = TYdbSetOperationBase<TMegamindSessionKey, TMegamindSessionData>;

    TMegamingSessionStoreYdbOperationBase()
        : TBase(&TMetrics::GetInstance().MegamindSessionMetrics.YdbSetMetrics)
    {
    }

protected:
    TString GetQueryTemplate() const override {
        // Session is loaded by |prev_req_id|, but saved both by |request_id|
        // and empty request id. We did this because some clients, like, YaBro
        // or Watches, do not send |prev_req_id|, or send it
        // sporadically. Therefore, to prevent session loss for such clients,
        // session is saved by empty request id to be loaded in the next
        // request in case of empty |prev_req_id|, and saved by correct
        // |request_id| to be loaded in the next request in case of correct
        // |prev_req_id|.
        static const TString QUERY_STORE = R"(
            DECLARE $shard_key as Uint64;
            DECLARE $key as String;
            DECLARE $data as String;
            DECLARE $updated_at as Uint64;
            DECLARE $ttl as Datetime;
            DECLARE $puid as String;

            UPSERT INTO
                data2
                (shard_key, key, data, updated_at, ttl, puid)
            VALUES
                ($shard_key, $key, $data, $updated_at, $ttl, $puid);
        )";
        return QUERY_STORE;
    }

    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override {
        return paramsBuilder
            .AddParam("$shard_key")
                .Uint64(Key.ShardKey)
                .Build()
            .AddParam("$key")
                .String(GetKey())
                .Build()
            .AddParam("$data")
                .String(Data.Data)
                .Build()
            .AddParam("$updated_at")
                .Uint64(TInstant::Now().Seconds())
                .Build()
            .AddParam("$ttl")
                .Datetime(TInstant::Now())
                .Build()
            .AddParam("$puid")
                .String(Data.Puid)
                .Build()
            .Build();
    }

public:
    virtual TString GetKey() const = 0;
};


class TMegamingSessionStoreYdbOperationOne : public TMegamingSessionStoreYdbOperationBase {
public:
    TString GetKey() const override {
        if (!PreparedKey.Defined()) {
            PreparedKey = CropKeyRequestId(Key.Key);
        }
        return PreparedKey.GetRef();
    }

private:
    mutable TMaybe<TString> PreparedKey;
};


class TMegamingSessionStoreYdbOperationTwo : public TMegamingSessionStoreYdbOperationBase {
public:
    TString GetKey() const override {
        return Key.Key;
    }
};


TMegamindSessionYdbStorage::TMegamindSessionYdbStorage(const NCachalot::TYdbSettings& settings)
    : Ydb{MakeIntrusive<TYdbContext>(settings)}
{
    Config.ReadTimeout = TDuration::Seconds(settings.ReadTimeoutSeconds());
    Config.WriteTimeout = TDuration::Seconds(settings.WriteTimeoutSeconds());

    if (settings.HasMaxRetriesCount()) {
        Config.MaxRetriesCount = settings.MaxRetriesCount();
    }
}

NThreading::TFuture<TMegamindSessionYdbStorage::TMultipleRowsResponse> TMegamindSessionYdbStorage::Get(
    const TMegamindSessionKey& key, TChroniclerPtr /* chronicler */
) {
    TMultipleRowsResponse response;
    response.SetKey(key);
    response.SetError("IStorage::Get method is not implemeted for TMegamindSessionYdbStorage");
    response.SetStatus(EResponseStatus::NOT_IMPLEMENTED);
    return NThreading::MakeFuture(response);
}

NThreading::TFuture<TMegamindSessionYdbStorage::TEmptyResponse> TMegamindSessionYdbStorage::Del(
    const TMegamindSessionKey& key, TChroniclerPtr /* chronicler */
) {
    TEmptyResponse response;
    response.SetKey(key);
    response.SetError("IStorage::Del method is not implemeted for TMegamindSessionYdbStorage");
    response.SetStatus(EResponseStatus::NOT_IMPLEMENTED);
    return NThreading::MakeFuture(response);
}

NThreading::TFuture<TMegamindSessionYdbStorage::TSingleRowResponse> TMegamindSessionYdbStorage::GetSingleRow(
    const TMegamindSessionKey& key, TChroniclerPtr chronicler
) {
    auto operation = MakeIntrusive<TMegamingSessionLoadYdbOperation>();
    operation->SetKey(key);
    operation->MutableOperationSettings().MaxRetries(Config.MaxRetriesCount)
                                         .OperationTimeout(Config.ReadTimeout)
                                         .ClientTimeout(Config.ReadTimeout + TDuration::MilliSeconds(100));
    operation->SetLogger(chronicler);
    return operation->Execute(Ydb->GetClient());
}

NThreading::TFuture<TMegamindSessionYdbStorage::TEmptyResponse> TMegamindSessionYdbStorage::Set(
    const TMegamindSessionKey& key, const TMegamindSessionData& data, int64_t ttl, TChroniclerPtr chronicler
) {
    Y_UNUSED(ttl);
    Y_ASSERT(chronicler != nullptr);

    TIntrusivePtr<TMegamingSessionStoreYdbOperationBase> one = MakeIntrusive<TMegamingSessionStoreYdbOperationOne>();
    TIntrusivePtr<TMegamingSessionStoreYdbOperationBase> two = MakeIntrusive<TMegamingSessionStoreYdbOperationTwo>();

    for (auto& operation : {one, two}) {
        operation->SetKey(key);
        operation->SetData(data);
        operation->MutableOperationSettings().MaxRetries(Config.MaxRetriesCount)
                                             .OperationTimeout(Config.WriteTimeout)
                                             .ClientTimeout(Config.WriteTimeout + TDuration::MilliSeconds(100));
        operation->SetLogger(chronicler);
    }

    NThreading::TPromise<TEmptyResponse> response = NThreading::NewPromise<TEmptyResponse>();

    chronicler->LogEvent(NEvClass::MMYdbSetStart());

    if (one->GetKey() != two->GetKey()) {
        SubscribeWithTryCatchWrapper<NEvClass::MMYdbSetErorr>(
            WaitBoth(
                one->Execute(Ydb->GetClient()),
                two->Execute(Ydb->GetClient())
            ),
            [response, chronicler](const auto& future) mutable {
                chronicler->LogEvent(NEvClass::MMYdbSetProcessing());

                const auto& [first, second] = future.GetValueSync();

                if (static_cast<int>(first.Status) < static_cast<int>(second.Status)) {
                    response.SetValue(second);
                } else {
                    response.SetValue(first);
                }
                chronicler->LogEvent(NEvClass::MMYdbSetFinish());
            },
            chronicler
        );
    } else {
        SubscribeWithTryCatchWrapper<NEvClass::MMYdbSetErorr>(
            one->Execute(Ydb->GetClient()),
            [response, chronicler](const auto& future) mutable {
                chronicler->LogEvent(NEvClass::MMYdbSetProcessing());
                response.SetValue(future.GetValueSync());
                chronicler->LogEvent(NEvClass::MMYdbSetFinish());
            },
            chronicler
        );
    }

    return response;
}


TMegamindSessionStorage::TBase::TStoragePtr TMegamindSessionStorage::GetLocalStorage() const {
    return TBase::LocalStorage;
}

}   // namespace NCachalot
