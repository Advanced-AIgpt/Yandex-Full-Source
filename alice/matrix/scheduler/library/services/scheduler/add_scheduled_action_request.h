#pragma once

#include <alice/matrix/scheduler/library/services/scheduler/protos/service.pb.h>

#include <alice/matrix/scheduler/library/storages/scheduler/storage.h>

#include <alice/matrix/library/request/typed_apphost_request.h>

namespace NMatrix::NScheduler {

class TAddScheduledActionRequest : public TTypedAppHostRequest<
    NServiceProtos::TAddScheduledActionRequest,
    NServiceProtos::TAddScheduledActionResponse,
    NEvClass::TMatrixSchedulerAddScheduledActionRequestData,
    NEvClass::TMatrixSchedulerAddScheduledActionResponseData
> {
public:
    TAddScheduledActionRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TAddScheduledActionRequest& request,
        TSchedulerStorage& schedulerStorage,
        const ui64 shardCount,
        const ui64 maxScheduledActionsToAddInOneAppHostRequest
    );

    NThreading::TFuture<void> ServeAsync() override;

public:
    static inline constexpr TStringBuf NAME = "add_scheduled_action";

private:
    const TExpected<TVector<TSchedulerStorage::TScheduledActionToAdd>, TString> ScheduledActionsToAdd_;

    TSchedulerStorage& SchedulerStorage_;
};

} // namespace NMatrix::NScheduler
