#include "storage.h"

#include <library/cpp/protobuf/interop/cast.h>

#include <util/digest/city.h>

namespace NMatrix::NScheduler {

TSchedulerStorage::TSchedulerStorage(
    const NYdb::TDriver& driver,
    const TYDBClientSettings& config
)
    : IYDBStorage(driver, config, NAME)
{}

NThreading::TFuture<TExpected<void, TString>> TSchedulerStorage::AddScheduledActions(
    const TVector<TScheduledActionToAdd>& scheduledActionsToAdd,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    if (scheduledActionsToAdd.empty()) {
        return NThreading::MakeFuture<TExpected<void, TString>>(TExpected<void, TString>::DefaultSuccess());
    }

    static const TString operationName = "add_scheduled_actions";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, scheduledActionsToAdd](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            DECLARE $created_at AS Timestamp;
            DECLARE $scheduled_actions_to_add AS List<
                Struct<
                    meta_action_id_hash: Uint64,
                    meta_action_id: String,

                    meta: String,
                    meta_action_guid: String,

                    spec: String,

                    status: String,
                    status_scheduled_at: Timestamp,

                    shard_id: Uint64,
                    override_mode: String,
                >
            >;

            DISCARD SELECT Ensure(
                0,
                q_data.override_mode != "NONE" OR db_data.meta_action_id_hash IS NULL,
                "Action with id " || q_data.meta_action_id || " already exists"
            )
            FROM AS_TABLE($scheduled_actions_to_add) AS q_data
            LEFT JOIN scheduled_actions AS db_data ON (
                q_data.meta_action_id_hash = db_data.meta_action_id_hash AND
                q_data.meta_action_id = db_data.meta_action_id
            );

            $real_scheduled_actions_to_add = (
                SELECT
                    q_data.meta_action_id_hash AS meta_action_id_hash,
                    q_data.meta_action_id AS meta_action_id,

                    q_data.meta AS meta,
                    q_data.meta_action_guid AS meta_action_guid,

                    q_data.spec AS spec,

                    IF (
                        q_data.override_mode = "ALL" OR db_data.status IS NULL,
                        q_data.status,
                        db_data.status
                    ) AS  status,
                    IF (
                        q_data.override_mode = "ALL" OR db_data.status_scheduled_at IS NULL,
                        q_data.status_scheduled_at,
                        db_data.status_scheduled_at
                    ) AS status_scheduled_at,

                    q_data.shard_id AS shard_id,
                    q_data.override_mode AS override_mode

                FROM AS_TABLE($scheduled_actions_to_add) AS q_data
                LEFT JOIN scheduled_actions AS db_data ON (
                    q_data.meta_action_id_hash = db_data.meta_action_id_hash AND
                    q_data.meta_action_id = db_data.meta_action_id
                )
            );

            UPSERT INTO scheduled_actions (
                SELECT
                    meta_action_id_hash AS meta_action_id_hash,
                    meta_action_id AS meta_action_id,

                    meta AS meta,
                    meta_action_guid AS meta_action_guid,

                    spec AS spec,

                    status AS status,
                    status_scheduled_at AS status_scheduled_at
                FROM $real_scheduled_actions_to_add
            );

            UPSERT INTO incoming_queue (
                SELECT
                    shard_id AS shard_id,
                    $created_at AS created_at,
                    meta_action_id AS action_id,
                    meta_action_guid AS action_guid,
                    status_scheduled_at AS scheduled_at
                FROM $real_scheduled_actions_to_add
            )
        )";


        TInstant now = TInstant::Now();
        auto paramsBuilder = session.GetParamsBuilder();

        paramsBuilder
            .AddParam("$created_at")
                .Timestamp(now)
                .Build()
        ;

        {
            auto& param = paramsBuilder.AddParam("$scheduled_actions_to_add");
            param.BeginList();
            for (const auto& scheduledActionToAdd : scheduledActionsToAdd) {
                static auto getOverrideMode = [](NApi::TAddScheduledActionRequest::EOverrideMode overrideMode) {
                    switch (overrideMode) {
                        case NApi::TAddScheduledActionRequest::NONE: {
                            return "NONE";
                        }
                        case NApi::TAddScheduledActionRequest::META_AND_SPEC_ONLY: {
                            return "META_AND_SPEC_ONLY";
                        }
                        case NApi::TAddScheduledActionRequest::ALL: {
                            return "ALL";
                        }
                        default: {
                            // Return NONE by default
                            return "NONE";
                        }
                    }
                };

                param.AddListItem()
                    .BeginStruct()
                        .AddMember("meta_action_id_hash").Uint64(CityHash64(scheduledActionToAdd.ScheduledAction.GetMeta().GetId()))
                        .AddMember("meta_action_id").String(scheduledActionToAdd.ScheduledAction.GetMeta().GetId())

                        .AddMember("meta").String(scheduledActionToAdd.ScheduledAction.GetMeta().SerializeAsString())
                        .AddMember("meta_action_guid").String(scheduledActionToAdd.ScheduledAction.GetMeta().GetGuid())

                        .AddMember("spec").String(scheduledActionToAdd.ScheduledAction.GetSpec().SerializeAsString())

                        .AddMember("status").String(scheduledActionToAdd.ScheduledAction.GetStatus().SerializeAsString())
                        .AddMember("status_scheduled_at").Timestamp(NProtoInterop::CastFromProto(scheduledActionToAdd.ScheduledAction.GetStatus().GetScheduledAt()))

                        .AddMember("shard_id").Uint64(scheduledActionToAdd.ShardId)
                        .AddMember("override_mode").String(getOverrideMode(scheduledActionToAdd.OverrideMode))
                    .EndStruct()
                ;
            }
            param.EndList().Build();
        }

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable {
        return operationContext.ReportResult(fut.GetValueSync());
    });
}

NThreading::TFuture<TExpected<void, TString>> TSchedulerStorage::RemoveScheduledActions(
    const TVector<TString>& scheduledActionIds,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    if (scheduledActionIds.empty()) {
        return NThreading::MakeFuture<TExpected<void, TString>>(TExpected<void, TString>::DefaultSuccess());
    }

    static const TString operationName = "remove_scheduled_actions";
    TOperationContext operationContext(
        Name_,
        operationName,
        OperationSettings_,
        logContext,
        metrics
    );

    return Client_.RetryOperation([this, scheduledActionIds](NYdb::NTable::TSession session) {
        static constexpr auto query = R"(
            PRAGMA AnsiInForEmptyOrNullableItemsCollections;

            DECLARE $scheduled_actions_to_remove AS List<
                Tuple<
                    Uint64,
                    String,
                >
            >;

            DELETE FROM scheduled_actions
            WHERE (meta_action_id_hash, meta_action_id) IN COMPACT $scheduled_actions_to_remove;
        )";

        auto paramsBuilder = session.GetParamsBuilder();

        {
            auto& param = paramsBuilder.AddParam("$scheduled_actions_to_remove");
            param.BeginList();
            for (const auto& scheduledActionId : scheduledActionIds) {
                param.AddListItem()
                    .BeginTuple()
                        .AddElement().Uint64(CityHash64(scheduledActionId))
                        .AddElement().String(scheduledActionId)
                    .EndTuple();
            }
            param.EndList().Build();
        }

        return session.ExecuteDataQuery(
            query,
            NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx(),
            paramsBuilder.Build(),
            ExecDataQuerySettings_
        ).Apply([](const NYdb::NTable::TAsyncDataQueryResult& fut) -> NYdb::TStatus {
            return fut.GetValueSync();
        });
    }, RetryOperationSettings_).Apply([operationContext = std::move(operationContext)](const NYdb::TAsyncStatus& fut) mutable {
        return operationContext.ReportResult(fut.GetValueSync());
    });
}

} // namespace NMatrix::NScheduler
