#pragma once

#include <alice/matrix/scheduler/library/services/scheduler/protos/service.pb.h>

#include <alice/matrix/scheduler/library/storages/scheduler/storage.h>

#include <alice/matrix/library/request/typed_apphost_request.h>

namespace NMatrix::NScheduler {

class TRemoveScheduledActionRequest : public TTypedAppHostRequest<
    NServiceProtos::TRemoveScheduledActionRequest,
    NServiceProtos::TRemoveScheduledActionResponse,
    NEvClass::TMatrixSchedulerRemoveScheduledActionRequestData,
    NEvClass::TMatrixSchedulerRemoveScheduledActionResponseData
> {
public:
    TRemoveScheduledActionRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        NAppHost::TTypedServiceContextPtr ctx,
        const NServiceProtos::TRemoveScheduledActionRequest& request,
        TSchedulerStorage& schedulerStorage,
        const ui64 maxScheduledActionsToRemoveInOneAppHostRequest
    );

    NThreading::TFuture<void> ServeAsync() override;

public:
    static inline constexpr TStringBuf NAME = "remove_scheduled_action";

private:
    const TExpected<TVector<TString>, TString> ScheduledActionsToRemove_;

    TSchedulerStorage& SchedulerStorage_;
};

} // namespace NMatrix::NScheduler
