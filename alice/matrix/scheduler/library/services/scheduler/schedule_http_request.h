#pragma once

#include <alice/matrix/scheduler/library/storages/scheduler/storage.h>

#include <alice/matrix/library/request/http_request.h>

#include <alice/protos/api/matrix/schedule_action.pb.h>

namespace NMatrix::NScheduler {

class TScheduleHttpRequest : public TProtoHttpRequest<
    NMatrix::NApi::TScheduleAction,
    NEvClass::TMatrixSchedulerScheduleHttpRequestData,
    NEvClass::TMatrixSchedulerScheduleHttpResponseData
> {
public:
    TScheduleHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TSchedulerStorage& schedulerStorage,
        const ui64 shardCount
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "schedule";
    static inline constexpr TStringBuf PATH = "/schedule";

private:
    TSchedulerStorage& SchedulerStorage_;
    const ui64 ShardCount_;

    const TExpected<NMatrix::NApi::TScheduledAction, TString> ScheduledAction_;
};

} // namespace NMatrix::NScheduler
