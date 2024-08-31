#include "subscriptions_http_request.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>


namespace NMatrix::NNotificator {

TSubscriptionsHttpRequest::TSubscriptionsHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TIoTClient& iotClient,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
)
    : THttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ true,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Get == method;
        }
    )
    , IoTClient_(iotClient)
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
    , Puid_()
    , UserTicket_(Nothing())
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

    if (const auto* userTicketHeader = HttpRequest_->Headers().FindHeader(TTvmClient::USER_TICKET_HEADER_NAME)) {
        UserTicket_ = userTicketHeader->Value();
    }

    if (Puid_.empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }
}

NThreading::TFuture<void> TSubscriptionsHttpRequest::ServeAsync() {
    auto userSubscriptionsFut = PushesAndNotificationsClient_.GetUserSubscriptionsForUser(Puid_, LogContext_, Metrics_);
    auto iotUserInfoFut = UserTicket_.Defined()
        ? IoTClient_.GetUserInfo(
            *UserTicket_,
            LogContext_,
            Metrics_
        )
        : NThreading::MakeFuture<TExpected<NAlice::TIoTUserInfo, TString>>(TExpected<NAlice::TIoTUserInfo, TString>::DefaultSuccess())
    ;

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            userSubscriptionsFut.Apply([](const auto&){}),
            iotUserInfoFut.Apply([](const auto&){}),
        })
    ).Apply(
        [this, userSubscriptionsFut, iotUserInfoFut](const auto&) {
            auto userSubscriptionsRes = userSubscriptionsFut.GetValueSync();
            auto iotUserInfoRes = iotUserInfoFut.GetValueSync();

            if (!userSubscriptionsRes) {
                SetError(userSubscriptionsRes.Error(), 500);
                return;
            }

            if (!iotUserInfoRes) {
                SetError(iotUserInfoRes.Error(), 500);
                return;
            }

            HttpReply_ = TReply(GetReplyData(userSubscriptionsRes.Success(), iotUserInfoRes.Success()), THttpHeaders(), 200);
        }
    );
}

bool TSubscriptionsHttpRequest::NeedTvmServiceTicket() const {
    return UserTicket_.Defined();
}

TSubscriptionsHttpRequest::TReply TSubscriptionsHttpRequest::GetReply() const {
    return HttpReply_;
}

TString TSubscriptionsHttpRequest::GetReplyData(
    const TMap<ui64, TSubscriptionsStorage::TUserSubscription>& userSubscriptions,
    const NAlice::TIoTUserInfo& iotUserInfo
) const {
    const auto& subscriptionsInfo = GetSubscriptionsInfo();

    NJson::TJsonMap response;

    THashSet<TString> iotDevicePlatforms;
    for (const auto& device : iotUserInfo.GetDevices()) {
        iotDevicePlatforms.insert(device.GetQuasarInfo().GetPlatform());
    }

    response["code"] = 200;
    {
        auto& payload = response["payload"] = NJson::TJsonMap();
        {
            auto& categories = payload["categories"] = NJson::TJsonArray();
            for (const auto& category : subscriptionsInfo.GetCategories()) {
                NJson::TJsonMap categoryMap;
                categoryMap["id"] = category.GetId();
                categoryMap["name"] = category.GetName();

                categories.AppendValue(categoryMap);
            }
        }

        {
            auto& subscriptions = payload["subscriptions"] = NJson::TJsonArray();
            for (const auto& [subscriptionId, subscriptionInfo] : subscriptionsInfo.GetSubscriptions()) {
                if (subscriptionInfo.GetType() != TSubscription::USER) {
                    continue;
                }

                if (UserTicket_.Defined()) {
                    bool hasAtLeastOneSuitableDevicePlatform = false;
                    for (const auto& platform : subscriptionInfo.GetSettings().GetPlatforms()) {
                        if (iotDevicePlatforms.contains(platform)) {
                            hasAtLeastOneSuitableDevicePlatform = true;
                            break;
                        }
                    }
                    if (!hasAtLeastOneSuitableDevicePlatform) {
                        continue;
                    }
                }

                NJson::TJsonMap subscriptionMap;

                subscriptionMap["id"] = subscriptionInfo.GetId();
                subscriptionMap["name"] = subscriptionInfo.GetName();
                subscriptionMap["description"] = subscriptionInfo.GetDescription();
                subscriptionMap["logo"] = subscriptionInfo.GetLogo();
                subscriptionMap["category"] = subscriptionInfo.GetCategory();
                subscriptionMap["type"] = static_cast<ui32>(subscriptionInfo.GetType());
                subscriptionMap["has_demo"] = subscriptionInfo.HasDemo();

                subscriptionMap["subscribed"] = userSubscriptions.contains(subscriptionInfo.GetId());

                {
                    auto& settings = subscriptionMap["settings"] = NJson::TJsonMap();

                    {
                        auto& deviceModels = settings["device_models"] = NJson::TJsonArray();
                        for (const auto& deviceModel : subscriptionInfo.GetSettings().GetDeviceModels()) {
                            deviceModels.AppendValue(deviceModel);
                        }
                    }

                    {
                        auto& platforms = settings["platforms"] = NJson::TJsonArray();
                        for (const auto& platform : subscriptionInfo.GetSettings().GetPlatforms()) {
                            if (UserTicket_.Defined() && !iotDevicePlatforms.contains(platform)) {
                                continue;
                            }
                            platforms.AppendValue(platform);
                        }
                    }
                }

                subscriptions.AppendValue(subscriptionMap);
            }
        }
    }

    return NJson::WriteJson(response, false, false, false);
}

} // namespace NMatrix::NNotificator
