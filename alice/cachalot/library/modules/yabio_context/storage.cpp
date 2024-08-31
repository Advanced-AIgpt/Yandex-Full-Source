#include <alice/cachalot/library/modules/yabio_context/storage.h>

#include <alice/cachalot/library/storage/mock.h>
#include <alice/cachalot/library/storage/ydb_operation.h>

#include <alice/cuttlefish/library/logging/dlog.h>
#include <library/cpp/digest/md5/md5.h>

#include <util/string/printf.h>

#include <stdlib.h>


using namespace NCachalot;


class TYabioContextSetOperation : public TYdbSetOperationBase<TYabioContextKey, TString> {
public:
    using TResponse = TYabioContextYdbStorage::TEmptyResponse;
    using TBase = TYdbSetOperationBase<TYabioContextKey, TString>;

    explicit TYabioContextSetOperation(TYabioContextSetYdbOperationSettings settings)
        : TBase(&(TMetrics::GetInstance().YabioContextServiceMetrics.YdbSet))
        , Settings(std::move(settings))
    {
        SetOperationSettings(Settings.Base());
    }

protected:
    TString GetQueryTemplate() const override;
    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override;

private:
    TYabioContextSetYdbOperationSettings Settings;
};

TString TYabioContextSetOperation::GetQueryTemplate() const {
    return Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $shard_key as Uint64;
        DECLARE $group_id as String;
        DECLARE $dev_model as String;
        DECLARE $dev_manuf as String;
        DECLARE $context as String;
        DECLARE $updated_at as Uint64;
        DECLARE $version as Uint32;
        UPSERT INTO yabio_storage_restyled (shard_key, group_id, dev_model, dev_manuf, context, updated_at, version)
        VALUES ($shard_key, $group_id, $dev_model, $dev_manuf, $context, $updated_at, $version);
    )", DatabaseName.c_str());
}

NYdb::TParams TYabioContextSetOperation::BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const {
    return paramsBuilder
        .AddParam("$shard_key").Uint64(Key.ShardKey).Build()
        .AddParam("$group_id").String(Key.GroupId).Build()
        .AddParam("$dev_model").String(Key.DevModel).Build()
        .AddParam("$dev_manuf").String(Key.DevManuf).Build()
        .AddParam("$context").String(Data).Build()
        .AddParam("$updated_at").Uint64(TInstant::Now().Seconds()).Build()
        .AddParam("$version").Uint32(1).Build()
        .Build();
}


class TYabioContextGetOperation :
    public TYdbOperationBase<TYabioContextYdbStorage::TSingleRowResponse, TYabioContextKey, TString>
{
public:
    using TResponse = TYabioContextYdbStorage::TSingleRowResponse;
    using TBase = TYdbOperationBase<TResponse, TYabioContextKey, TString>;

    explicit TYabioContextGetOperation(TYabioContextGetSingleRowYdbOperationSettings settings)
        : TBase(&(TMetrics::GetInstance().YabioContextServiceMetrics.YdbGet))
        , Settings(std::move(settings))
    {
        SetOperationSettings(Settings.Base());
    }

protected:
    TString GetQueryTemplate() const override;
    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override;
    void ProcessQueryResult(
        NThreading::TPromise<NYdb::TStatus>* status,
        const NYdb::NTable::TDataQueryResult& queryResult) override;

private:
    TYabioContextGetSingleRowYdbOperationSettings Settings;
};

TString TYabioContextGetOperation::GetQueryTemplate() const {
    return Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $shard_key AS Uint64;
        DECLARE $group_id as String;
        DECLARE $dev_model as String;
        DECLARE $dev_manuf as String;

        SELECT context, updated_at, version
        FROM %s
        WHERE shard_key = $shard_key AND
            group_id = $group_id AND
            dev_model = $dev_model AND
            dev_manuf = $dev_manuf
        LIMIT 1
    )", DatabaseName.c_str(), Settings.Base().Table().c_str());
}

NYdb::TParams TYabioContextGetOperation::BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const {
    return paramsBuilder
        .AddParam("$shard_key").Uint64(Key.ShardKey).Build()
        .AddParam("$group_id").String(Key.GroupId).Build()
        .AddParam("$dev_model").String(Key.DevModel).Build()
        .AddParam("$dev_manuf").String(Key.DevManuf).Build()
        .Build();
}

void TYabioContextGetOperation::ProcessQueryResult(
    NThreading::TPromise<NYdb::TStatus>* status,
    const NYdb::NTable::TDataQueryResult& queryResult
) {
    LastResponse.SetKey(Key);
    LastResponse.SetStats(Stats);

    if (queryResult.IsSuccess()) {
        if (queryResult.GetResultSets().size() != 1) {
            LastResponse.SetStatus(EResponseStatus::INTERNAL_ERROR);
            Metrics->OnNotFound(StartTime);
        } else {
            NYdb::TResultSetParser parser = queryResult.GetResultSetParser(0);
            if (parser.TryNextRow()) {
                LastResponse.SetData(
                    parser.ColumnParser("context").GetOptionalString().GetOrElse("")
                );
                LastResponse.SetStatus(EResponseStatus::OK);
                Metrics->OnOk(StartTime);
            } else {
                LastResponse.SetStatus(EResponseStatus::NOT_FOUND);
                Metrics->OnNotFound(StartTime);
            }
        }
    } else {
        LastResponse.SetStatus(EResponseStatus::QUERY_EXECUTE_FAILED).SetError(queryResult.GetIssues().ToString());
        Metrics->OnExecuteError(StartTime);
    }
    status->SetValue(queryResult);
}


class TYabioContextDeleteOperation : public TYdbDeleteOperationBase<TYabioContextKey> {
public:
    using TResponse = TYabioContextYdbStorage::TEmptyResponse;
    using TBase = TYdbDeleteOperationBase<TYabioContextKey>;

    explicit TYabioContextDeleteOperation(TYabioContextDelYdbOperationSettings settings)
        : TBase(&(TMetrics::GetInstance().YabioContextServiceMetrics.YdbDel))
        , Settings(std::move(settings))
    {
        SetOperationSettings(Settings.Base());
    }

protected:
    TString GetQueryTemplate() const override;
    NYdb::TParams BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const override;

private:
    TYabioContextDelYdbOperationSettings Settings;
};

TString TYabioContextDeleteOperation::GetQueryTemplate() const {
    if (!Key.IsFull()) {
        return Sprintf(R"(
            --!syntax_v1
            PRAGMA TablePathPrefix("%s");
            DECLARE $group_id as String;

            $to_delete = (
                SELECT * FROM %s VIEW ix_yabio_group_id_async
                WHERE group_id = $group_id
            );

            DELETE FROM %s ON
                SELECT * FROM $to_delete;
        )", DatabaseName.c_str(), Settings.Base().Table().c_str(), Settings.Base().Table().c_str());
    }

    return Sprintf(R"(
        --!syntax_v1
        PRAGMA TablePathPrefix("%s");
        DECLARE $shard_key AS Uint64;
        DECLARE $group_id as String;
        DECLARE $dev_model as String;
        DECLARE $dev_manuf as String;

        DELETE FROM %s
            WHERE shard_key = $shard_key AND
                group_id = $group_id AND
                dev_model = $dev_model AND
                dev_manuf = $dev_manuf;
    )", DatabaseName.c_str(), Settings.Base().Table().c_str());
}

NYdb::TParams TYabioContextDeleteOperation::BuildQueryParams(NYdb::TParamsBuilder paramsBuilder) const {
    if (!Key.IsFull()) {
        return paramsBuilder
            .AddParam("$group_id").String(Key.GroupId).Build()
            .Build();
    }

    return paramsBuilder
        .AddParam("$shard_key").Uint64(Key.ShardKey).Build()
        .AddParam("$group_id").String(Key.GroupId).Build()
        .AddParam("$dev_model").String(Key.DevModel).Build()
        .AddParam("$dev_manuf").String(Key.DevManuf).Build()
        .Build();
}


namespace NCachalot {

    TYabioContextKey::TYabioContextKey(const NCachalotProtocol::TYabioContextKey& key) {
        if (key.HasGroupId()) {
            GroupId = key.GetGroupId();
        }
        if (key.HasDevModel()) {
            DevModel = key.GetDevModel();
        }
        if (key.HasDevManuf()) {
            DevManuf = key.GetDevManuf();
        }
        Normalize();
        ComputeShardKey();
    }

    void TYabioContextKey::Normalize() {
        GroupId.to_lower();
        DevModel.to_lower();
        DevManuf.to_lower();
    }

    void TYabioContextKey::ComputeShardKey() {
        TStringStream ss;
        ss << GroupId;
        if (DevModel) {
            if (ss.Str()) {
                ss << '_';
            }
            ss << DevModel;
        }
        if (DevManuf) {
            if (ss.Str()) {
                ss << '_';
            }
            ss << DevManuf;
        }
        const TString md5LastHalf = MD5::Calc(ss.Str()).substr(16);
        ShardKey = std::strtoull(md5LastHalf.data(), nullptr, 16);
    }

    void TYabioContextKey::SaveTo(NCachalotProtocol::TYabioContextKey& key) const {
        key.SetGroupId(GroupId);
        key.SetDevModel(DevModel);
        key.SetDevManuf(DevManuf);
    }

    bool TYabioContextKey::IsFull() const {
        return !(DevManuf == "" || DevModel == "");
    }

    TString TYabioContextKey::AsString() const {
        return TStringBuilder() << GroupId << '_' << '_' << DevModel << '_' << '_' << DevManuf;
    }


    TYabioContextYdbStorage::TYabioContextYdbStorage(const TYabioContextStorageSettings& settings)
        : Ydb(settings.YdbClient())
        , Settings(settings)
    {
    }

    NThreading::TFuture<TYabioContextYdbStorage::TEmptyResponse> TYabioContextYdbStorage::Set(
        const TYabioContextKey& key,
        const TString& data,
        int64_t ttl,
        TChroniclerPtr chronicler
    ) {
        Y_UNUSED(ttl);


        int64_t maxTTL = static_cast<int64_t>(Ydb.GetSettings().MaxTimeToLiveSeconds());
        if (ttl > maxTTL) {
            ttl = maxTTL;
        }

        auto operation = MakeIntrusive<TYabioContextSetOperation>(Settings.Save());
        operation->SetTtl(ttl);
        operation->SetKey(key);
        operation->SetData(data);
        operation->SetDatabase(Ydb.GetSettings().Database());
        operation->SetLogger(chronicler);
        return operation->Execute(Ydb.GetClient());
    }

    NThreading::TFuture<TYabioContextYdbStorage::TSingleRowResponse> TYabioContextYdbStorage::GetSingleRow(
        const TYabioContextKey& key,
        TChroniclerPtr chronicler
    ) {
        auto operation = MakeIntrusive<TYabioContextGetOperation>(Settings.Load());
        operation->SetKey(key);
        operation->SetDatabase(Ydb.GetSettings().Database());
        operation->SetLogger(chronicler);
        return operation->Execute(Ydb.GetClient());
    }

    NThreading::TFuture<TYabioContextYdbStorage::TMultipleRowsResponse> TYabioContextYdbStorage::Get(
        const TYabioContextKey& key,
        TChroniclerPtr
    ) {
        TMultipleRowsResponse response;
        response.SetKey(key);
        response.SetError("IStorage::Get(MultipleRows) method is not implemeted for TYabioContextYdbStorage");
        response.SetStatus(EResponseStatus::NOT_IMPLEMENTED);
        return NThreading::MakeFuture(response);
    }

    NThreading::TFuture<TYabioContextYdbStorage::TEmptyResponse> TYabioContextYdbStorage::Del(
        const TYabioContextKey& key,
        TChroniclerPtr chronicler
    ) {
        auto operation = MakeIntrusive<TYabioContextDeleteOperation>(Settings.Remove());
        operation->SetKey(key);
        operation->SetDatabase(Ydb.GetSettings().Database());
        operation->SetLogger(chronicler);
        return operation->Execute(Ydb.GetClient());
    }


    TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> MakeYabioContextYdbStorage(
        const NCachalot::TYabioContextStorageSettings& settings
    ) {
        return MakeStorage<TYabioContextYdbStorage>(settings.YdbClient().IsFake(), settings);
    }

}  // namespace NCachalot
