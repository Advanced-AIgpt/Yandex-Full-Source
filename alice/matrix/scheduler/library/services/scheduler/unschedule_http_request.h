#pragma once

#include <alice/matrix/scheduler/library/storages/scheduler/storage.h>

#include <alice/matrix/library/request/http_request.h>

#include <alice/protos/api/matrix/schedule_action.pb.h>

namespace NMatrix::NScheduler {

class TUnscheduleHttpRequest : public TProtoHttpRequest<
    NMatrix::NApi::TRemoveAction,
    NEvClass::TMatrixSchedulerUnscheduleHttpRequestData,
    NEvClass::TMatrixSchedulerUnscheduleHttpResponseData
> {
public:
    TUnscheduleHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TSchedulerStorage& schedulerStorage
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "unschedule";
    static inline constexpr TStringBuf PATH = "/unschedule";

private:
    TSchedulerStorage& SchedulerStorage_;
};

} // namespace NMatrix::NScheduler
