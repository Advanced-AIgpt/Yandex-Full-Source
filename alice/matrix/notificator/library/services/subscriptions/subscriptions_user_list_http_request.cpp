#include "subscriptions_user_list_http_request.h"

#include <alice/matrix/notificator/library/utils/utils.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>


namespace NMatrix::NNotificator {

TSubscriptionsUserListHttpRequest::TSubscriptionsUserListHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
)
    : THttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ false,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Get == method;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
    , SubscriptionId_(0)
    , AfterTimestamp_(Nothing())
{
    if (IsFinished()) {
        return;
    }

    const TCgiParameters cgi(HttpRequest_->Cgi());

    if (const auto subscriptionIdIt = cgi.Find("subscription_id"); subscriptionIdIt != cgi.end()) {
        if (auto parseResult = TryParseFromString(subscriptionIdIt->second, "subscription id")) {
            SubscriptionId_ = parseResult.Success();
        } else {
            SetError(parseResult.Error(), 400);
            IsFinished_ = true;
            return;
        }
    } else {
        SetError("'subscription_id' param not found", 400);
        IsFinished_ = true;
        return;
    }

    if (const auto afterTimestampIt = cgi.Find("timestamp"); afterTimestampIt != cgi.end()) {
        if (auto parseResult = TryParseFromString(afterTimestampIt->second, "timestamp")) {
            AfterTimestamp_ = parseResult.Success();
        } else {
            SetError(parseResult.Error(), 400);
            IsFinished_ = true;
            return;
        }
    }
}

NThreading::TFuture<void> TSubscriptionsUserListHttpRequest::ServeAsync() {
    return PushesAndNotificationsClient_.GetUserSubscriptionsBySubscriptionId(
        SubscriptionId_,
        AfterTimestamp_,
        LogContext_,
        Metrics_
    ).Apply(
        [this](
            const NThreading::TFuture<TExpected<TVector<TSubscriptionsStorage::TUserSubscription>, TString>>& userSubscriptionsFut
        ) {
            const auto& userSubscriptionsRes = userSubscriptionsFut.GetValueSync();
            if (!userSubscriptionsRes) {
                SetError(userSubscriptionsRes.Error(), 500);
                return;
            }

            HttpReply_ = TReply(GetReplyData(userSubscriptionsRes.Success()), THttpHeaders(), 200);
        }
    );
}

TSubscriptionsUserListHttpRequest::TReply TSubscriptionsUserListHttpRequest::GetReply() const {
    return HttpReply_;
}

TString TSubscriptionsUserListHttpRequest::GetReplyData(const TVector<TSubscriptionsStorage::TUserSubscription>& userSubscriptions) const {
    NJson::TJsonMap response;

    response["code"] = (userSubscriptions.empty() ? 404 : 200);
    {
        auto& payload = response["payload"] = NJson::TJsonMap();
        {
            auto& users = payload["users"] = NJson::TJsonArray();
            ui64 maxTimestamp = 0;
            for (const auto& userSubscription : userSubscriptions) {
                users.AppendValue(userSubscription.Puid);
                maxTimestamp = Max(maxTimestamp, userSubscription.SubscribedAtTimestamp);
            }
            payload["timestamp"] = maxTimestamp;
        }
    }

    return NJson::WriteJson(response, false, false, false);
}

} // namespace NMatrix::NNotificator
