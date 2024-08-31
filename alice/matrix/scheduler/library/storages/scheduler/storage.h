#pragma once

#include <alice/protos/api/matrix/scheduled_action.pb.h>
#include <alice/protos/api/matrix/scheduler_api.pb.h>

#include <alice/matrix/library/ydb/storage.h>

#include <alice/protos/api/matrix/schedule_action.pb.h>

namespace NMatrix::NScheduler {

class TSchedulerStorage : public IYDBStorage {
public:
    struct TScheduledActionToAdd {
        ui64 ShardId;
        NApi::TAddScheduledActionRequest::EOverrideMode OverrideMode;
        NMatrix::NApi::TScheduledAction ScheduledAction;
    };

public:
    TSchedulerStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    NThreading::TFuture<TExpected<void, TString>> AddScheduledActions(
        const TVector<TScheduledActionToAdd>& scheduledActionsToAdd,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<void, TString>> RemoveScheduledActions(
        const TVector<TString>& scheduledActionIds,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    static inline constexpr TStringBuf NAME = "scheduler";
};

} // namespace NMatrix::NScheduler
