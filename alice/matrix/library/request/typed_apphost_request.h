#pragma once

#include "request.h"
#include "request_event_patcher.h"

#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

#include <apphost/api/service/cpp/service_exceptions.h>
#include <apphost/api/service/cpp/typed_service_context.h>

namespace NMatrix {

template <
    typename TRequest,
    typename TResponse,
    typename TRequestDataEvent,
    typename TResponseDataEvent,
    TRequestEventPatcher<TRequestDataEvent> RequestDataEventPatcher = EmptyRequestEventPatcher<TRequestDataEvent>,
    TRequestEventPatcher<TResponseDataEvent> ResponseDataEventPatcher = EmptyRequestEventPatcher<TResponseDataEvent>
>
class TTypedAppHostRequest : public IRequest<TResponse> {
public:
    TTypedAppHostRequest(
        const TStringBuf name,
        std::atomic<size_t>& requestCounterRef,
        const bool needThreadSafeLogFrame,
        TRtLogClient& rtLogClient,
        NAppHost::TTypedServiceContextPtr ctx,
        const TRequest& request
    )
        : IRequest<TResponse>(
            name,
            requestCounterRef,
            needThreadSafeLogFrame,
            rtLogClient
        )
        , Request_(request)
        , Response_()
        , Ctx_(ctx)
    {
        this->InitRtLog(GetRtLogTokenFromTypedAppHostServiceContext(*Ctx_));

        this->LogContext_.template LogEventInfoCombo<NEvClass::TMatrixAppHostRequestMetaInfo>(
            GuidToUuidString(Ctx_->GetGUID()),
            Ctx_->GetRUID(),
            Ctx_->GetPathInGraph(),
            TString(name),
            Ctx_->GetRemoteHost(),
            Ctx_->GetRequestID(),
            ToString(Ctx_->GetRequestAttemptType())
        );

        {
            TRequestDataEvent event;
            event.MutableRequest()->CopyFrom(Request_);
            RequestDataEventPatcher(event);
            this->LogContext_.template LogEventInfoCombo<TRequestDataEvent>(event);
        }
    }

    NThreading::TFuture<TResponse> ReplyWithFutureCheck(const NThreading::TFuture<void>& rspFut) override final {
        try {
            rspFut.GetValueSync();
        } catch (...) {
            SetError(CurrentExceptionMessage());
        }

        return Reply();
    }

    NThreading::TFuture<TResponse> Reply() override final {
        TResponseDataEvent event;

        if (ErrorMessage_) {
            this->Metrics_.RateAppHostResponseError();

            event.SetErrorMessage(*ErrorMessage_);
            ResponseDataEventPatcher(event);
            this->LogContext_.template LogEventErrorCombo<TResponseDataEvent>(event);

            return NThreading::MakeErrorFuture<TResponse>(std::make_exception_ptr(NAppHost::NService::TRequestError() << *ErrorMessage_));
        } else {
            this->Metrics_.RateAppHostResponseOk();

            event.MutableResponse()->CopyFrom(Response_);
            ResponseDataEventPatcher(event);
            this->LogContext_.template LogEventInfoCombo<TResponseDataEvent>(event);

            return NThreading::MakeFuture<TResponse>(Response_);
        }
    }

protected:
    void SetError(const TString& message) {
        ErrorMessage_ = message;
    }

protected:
    const TRequest& Request_;
    TResponse Response_;

private:
    NAppHost::TTypedServiceContextPtr Ctx_;
    TMaybe<TString> ErrorMessage_;
};

} // namespace NMatrix
