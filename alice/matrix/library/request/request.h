#pragma once

#include <alice/matrix/library/logging/log_context.h>
#include <alice/matrix/library/metrics/metrics.h>

#include <library/cpp/threading/future/future.h>

namespace NMatrix {

template <typename TReplyType>
class IRequest : public TThrRefBase {
public:
    IRequest(
        const TStringBuf name,
        std::atomic<size_t>& requestCounterRef,
        const bool needThreadSafeLogFrame,
        TRtLogClient& rtLogClient
    )
        : Metrics_(name)
        , LogContext_(SpawnLogFrame(needThreadSafeLogFrame), /* rtlog = */ nullptr)
        , IsFinished_(false)
        , StartTime_(TInstant::Now())
        , RequestCounterRef_(requestCounterRef)
        , RtLogClient_(rtLogClient)
    {
        ++RequestCounterRef_;
    }

    virtual ~IRequest() {
        try {
            Metrics_.PushTimeDiffWithNowHist(
                StartTime_,
                "request_time"
            );
        } catch (...) {
        }

        --RequestCounterRef_;
    }

    // True if already replied.
    bool IsFinished() const {
        return IsFinished_;
    }

    virtual NThreading::TFuture<TReplyType> ReplyWithFutureCheck(const NThreading::TFuture<void>& rspFut) = 0;
    virtual NThreading::TFuture<TReplyType> Reply() = 0;

    virtual NThreading::TFuture<void> ServeAsync() = 0;

protected:
    void InitRtLog(const TString& rtLogToken) {
        LogContext_.ResetRtLogger(RtLogClient_.CreateRequestLogger(rtLogToken));
    }

protected:
    mutable NMatrix::TSourceMetrics Metrics_;
    TLogContext LogContext_;

    bool IsFinished_;
    const TInstant StartTime_;

private:
    std::atomic<size_t>& RequestCounterRef_;

    TRtLogClient& RtLogClient_;
};

} // namespace NMatrix
