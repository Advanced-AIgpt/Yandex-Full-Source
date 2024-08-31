#pragma once

#include <alice/matrix/notificator/library/pushes_and_notifications/client.h>

#include <alice/matrix/library/request/http_request.h>

#include <alice/uniproxy/library/protos/notificator.pb.h>


namespace NMatrix::NNotificator {

class TSubscriptionsManageHttpRequest : public THttpRequest<
    // This request is bad
    // It has three versions: cgi, json and protobuf
    // and all of them are used in production

    /* TRequestDataEvent = */ NPrivate::TFakeDataEvent,
    NEvClass::TMatrixNotificatorSubscriptionsManageHttpResponseData,
    /* RequestDataEventPatcher = */ EmptyRequestEventPatcher<NPrivate::TFakeDataEvent>,
    EmptyRequestEventPatcher<NEvClass::TMatrixNotificatorSubscriptionsManageHttpResponseData>,

    // Do not log raw request data
    // Special logging is implemented in this class
    /* LogRawRequestData = */ false,

    /* LogRawResponseData = */ true
> {
public:
    TSubscriptionsManageHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        TPushesAndNotificationsClient& pushesAndNotificationsClient
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

    TString GetReplyData() const;

public:
    static inline constexpr TStringBuf NAME = "subscriptions_manage";
    static inline constexpr TStringBuf PATH = "/subscriptions/manage";

private:
    TPushesAndNotificationsClient& PushesAndNotificationsClient_;

    ::NNotificator::TManageSubscription Request_;
};

} // namespace NMatrix::NNotificator
