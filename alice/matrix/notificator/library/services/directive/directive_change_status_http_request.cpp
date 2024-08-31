#include "directive_change_status_http_request.h"

namespace NMatrix::NNotificator {

TDirectiveChangeStatusHttpRequest::TDirectiveChangeStatusHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TDirectivesStorage& directivesStorage
)
    : TProtoHttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , DirectivesStorage_(directivesStorage)
{
    if (IsFinished()) {
        return;
    }

    for (const auto& pushId : Request_->GetIds()) {
        // Important log for analytics
        LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorChangeDirectiveStatus>(
            pushId,
            static_cast<ui64>(Request_->GetStatus()),
            Request_->GetPuid(),
            Request_->GetDeviceId()
        );
    }

    static const auto* descriptor = NAlice::NNotificator::EDirectiveStatus_descriptor();
    const auto status = to_lower(descriptor->FindValueByNumber(Request_->GetStatus())->name());
    Metrics_.PushRate(TString::Join(status, "_status_requests_count"));
    Metrics_.PushRate(Request_->GetIds().size(), TString::Join(status, "_status_directives_count"));

    if (Request_->GetStatus() == NAlice::NNotificator::EDirectiveStatus::ED_DELETED) {
        SetError("'ED_DELETED' status in request is not supported", 400);
        IsFinished_ = true;
        return;
    }

    // TODO(ZION-289) no other status than ED_DELIVERED is allowed here
    if (Request_->GetStatus() == NAlice::NNotificator::EDirectiveStatus::ED_DELIVERED) {
        for (const auto& PushId : Request_->GetIds()) {
            LogContext_.LogEventInfoCombo<NEvClass::TMatrixNotificatorAnalyticsTechnicalPushDeliveryAcknowledge>(
                PushId
            );
        }
    }

    if (Request_->GetIds().empty()) {
        Metrics_.PushRate("empty_push_ids_requests_count");
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TDirectiveChangeStatusHttpRequest::ServeAsync() {
    return DirectivesStorage_.ChangeDirectivesStatus(*Request_, LogContext_, Metrics_).Apply(
        [this](const NThreading::TFuture<TExpected<void, TString>>& fut) {
            const auto& res = fut.GetValueSync();
            if (!res) {
                SetError(res.Error(), 500);
            }
        }
    );
}

TDirectiveChangeStatusHttpRequest::TReply TDirectiveChangeStatusHttpRequest::GetReply() const {
    return TReply(200);
}

} // namespace NMatrix::NNotificator
