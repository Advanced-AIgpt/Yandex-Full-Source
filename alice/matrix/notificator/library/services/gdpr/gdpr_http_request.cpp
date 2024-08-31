#include "gdpr_http_request.h"

#include <library/cpp/cgiparam/cgiparam.h>


namespace NMatrix::NNotificator {

TGDPRHttpRequest::TGDPRHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
)
    : THttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ true,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Delete == method;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
    , Puid_()
{
    if (IsFinished()) {
        return;
    }

    const TCgiParameters cgi(HttpRequest_->Cgi());

    if (const auto puidIt = cgi.Find("puid"); puidIt != cgi.end()) {
        Puid_ = puidIt->second;
    } else {
        SetError("'puid' param not found", 400);
        IsFinished_ = true;
        return;
    }

    if (Puid_.empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TGDPRHttpRequest::ServeAsync() {
    return PushesAndNotificationsClient_.RemoveAllUserData(
        Puid_,
        LogContext_,
        Metrics_
    ).Apply(
        [this](const auto& fut) -> NThreading::TFuture<void> {
            auto futRes = fut.GetValueSync();
            if (!futRes) {
                SetError(futRes.Error(), 500);
                return NThreading::MakeFuture();
            }

            return PushesAndNotificationsClient_.ActualizeUserNotificationsInfoAndSendItToDevices(
                Puid_,
                /* deviceId = */ Nothing(),
                /* forceNotificationsStateUpdate = */ true,
                /* sendEmptyState = */ true,
                NAlice::NScenarios::TNotifyDirective::NoSound,
                /* listConnectionsFilter = */ Nothing(),
                LogContext_,
                Metrics_
            ).Apply(
                [this](const NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>& fut) mutable {
                    const auto& futRes = fut.GetValueSync();
                    if (!futRes) {
                        SetError(futRes.Error(), 500);
                        return;
                    }
                }
            );
        }
    );
}

TGDPRHttpRequest::TReply TGDPRHttpRequest::GetReply() const {
    return TReply("", THttpHeaders(), 200);
}

} // namespace NMatrix::NNotificator
