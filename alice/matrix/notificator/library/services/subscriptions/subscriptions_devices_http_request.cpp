#include "subscriptions_devices_http_request.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>

#include <alice/protos/data/location/room.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>


namespace NMatrix::NNotificator {

TSubscriptionsDevicesHttpRequest::TSubscriptionsDevicesHttpRequest(
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
            return
                NNeh::NHttp::ERequestType::Get == method ||
                NNeh::NHttp::ERequestType::Post == method
            ;
        }
    )
    , IoTClient_(iotClient)
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
    , SubscriptionMethod_(ESubscriptionMethod::SUBSCRIBE)
{
    if (IsFinished()) {
        return;
    }

    NEvClass::TMatrixNotificatorSubscriptionsDevicesHttpRequestData event;
    TMaybe<TStringBuilder> parseError = Nothing();
    auto updateParseError = [&parseError](const TString& error) {
        if (!parseError.Defined()) {
            parseError.ConstructInPlace();
            *parseError << error;
        } else {
            *parseError << TString::Join("; ", error);
        }
    };

    if (NNeh::NHttp::ERequestType::Get == Method_) {
        const TCgiParameters cgi(HttpRequest_->Cgi());

        if (const auto puidIt = cgi.Find("puid"); puidIt != cgi.end()) {
            Puid_ = puidIt->second;
        } else {
            updateParseError("'puid' param not found");
        }

        if (const auto* userTicketHeader = HttpRequest_->Headers().FindHeader(TTvmClient::USER_TICKET_HEADER_NAME)) {
            UserTicket_ = userTicketHeader->Value();
        } else {
            updateParseError("User ticket not provided");
        }

        if (!parseError.Defined()) {
            event.SetGetRequest(TString(HttpRequest_->Body()));
        }

    } else {
        try {
            NJson::TJsonValue jsonBody;
            NJson::ReadJsonTree(HttpRequest_->Body(), &jsonBody, true);

            if (const auto* puid = jsonBody.GetValueByPath("puid"); puid && puid->IsString()) {
                Puid_ = puid->GetString();
            } else {
                updateParseError("'puid' param not found");
            }

            if (const auto* deviceId = jsonBody.GetValueByPath("device_id"); deviceId && deviceId->IsString()) {
                DeviceId_ = deviceId->GetString();
            } else {
                updateParseError("'device_id' param not found");
            }

            if (const auto* method = jsonBody.GetValueByPath("method"); method && method->IsString()) {
                // As is from python https://a.yandex-team.ru/svn/trunk/arcadia/alice/uniproxy/library/notificator/subscribes.py?rev=r9691064#L333
                // Do not compare with unsubscribe
                SubscriptionMethod_ = method->GetString() == "subscribe"
                    ? ESubscriptionMethod::SUBSCRIBE
                    : ESubscriptionMethod::UNSUBSCRIBE
                ;
            } else {
                updateParseError("'method' param not found");
            }

        } catch (...) {
            updateParseError(CurrentExceptionMessage());
        }

        if (!parseError.Defined()) {
            event.SetManageRequest(TString(HttpRequest_->Body()));
        }
    }

    if (parseError.Defined()) {
        event.SetUnparsedRawRequest(TString(HttpRequest_->Body()));
    }

    LogContext_.LogEventInfo<NEvClass::TMatrixNotificatorSubscriptionsDevicesHttpRequestData>(event);

    if (parseError) {
        SetError(*parseError, 400);
        IsFinished_ = true;
        return;
    }

    if (
        const auto* serviceTicketHeader = HttpRequest_->Headers().FindHeader(TTvmClient::SERVICE_TICKET_HEADER_NAME);
        !serviceTicketHeader || serviceTicketHeader->Value().empty()
    ) {
        SetError("Service ticket not provided", 403);
        IsFinished_ = true;
        return;
    }

    if (Puid_.empty()) {
        SetError("Puid is empty", 400);
        IsFinished_ = true;
        return;
    }

    if (NNeh::NHttp::ERequestType::Get == Method_) {
        // Special checks for get request

        // Nothing to check
    } else {
        // Special checks for manage request

        if (DeviceId_.empty()) {
            SetError("Device id is empty", 400);
            IsFinished_ = true;
            return;
        }
    }
}

NThreading::TFuture<void> TSubscriptionsDevicesHttpRequest::ServeAsync() {
    if (NNeh::NHttp::ERequestType::Get == Method_) {
        return ProcessGetRequest();
    } else {
        return ProcessManageRequest();
    }
}

bool TSubscriptionsDevicesHttpRequest::NeedTvmServiceTicket() const {
    return NNeh::NHttp::ERequestType::Get == Method_;
}

TSubscriptionsDevicesHttpRequest::TReply TSubscriptionsDevicesHttpRequest::GetReply() const {
    return HttpReply_;
}

NThreading::TFuture<void> TSubscriptionsDevicesHttpRequest::ProcessGetRequest() {
    auto userUnsubscribedDevicesFut = PushesAndNotificationsClient_.GetUserUnsubscribedDevices(Puid_, LogContext_, Metrics_);
    auto iotUserInfoFut = IoTClient_.GetUserInfo(UserTicket_, LogContext_, Metrics_);

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            userUnsubscribedDevicesFut.Apply([](const auto&){}),
            iotUserInfoFut.Apply([](const auto&){}),
        })
    ).Apply(
        [this, userUnsubscribedDevicesFut, iotUserInfoFut](const auto&) {
            auto userUnsubscribedDevicesRes = userUnsubscribedDevicesFut.GetValueSync();
            auto iotUserInfoRes = iotUserInfoFut.GetValueSync();

            if (!userUnsubscribedDevicesRes) {
                SetError(userUnsubscribedDevicesRes.Error(), 500);
                return;
            }

            if (!iotUserInfoRes) {
                SetError(iotUserInfoRes.Error(), 500);
                return;
            }

            HttpReply_ = TReply(GetReplyDataForGetRequest(userUnsubscribedDevicesRes.Success(), iotUserInfoRes.Success()), THttpHeaders(), 200);
        }
    );
}

NThreading::TFuture<void> TSubscriptionsDevicesHttpRequest::ProcessManageRequest() {
    NThreading::TFuture<TExpected<void, TString>> subOrUnsubUserDeviceFut;
    switch (SubscriptionMethod_) {
        case ESubscriptionMethod::SUBSCRIBE: {
            subOrUnsubUserDeviceFut = PushesAndNotificationsClient_.SubscribeUserDevice(
                Puid_,
                DeviceId_,
                LogContext_,
                Metrics_
            );
            break;
        }
        case ESubscriptionMethod::UNSUBSCRIBE: {
            subOrUnsubUserDeviceFut = PushesAndNotificationsClient_.UnsubscribeUserDevice(
                Puid_,
                DeviceId_,
                LogContext_,
                Metrics_
            );
            break;
        }
    }

    return subOrUnsubUserDeviceFut.Apply(
       [this](const NThreading::TFuture<TExpected<void, TString>>& subOrUnsubUserDeviceFut) -> NThreading::TFuture<void> {
            const auto& subOrUnsubUserDeviceRes = subOrUnsubUserDeviceFut.GetValueSync();
            if (!subOrUnsubUserDeviceRes) {
                SetError(subOrUnsubUserDeviceRes.Error(), 500);
                return NThreading::MakeFuture();
            }

            return PushesAndNotificationsClient_.ActualizeUserNotificationsInfoAndSendItToDevices(
                Puid_,
                DeviceId_,
                /* forceNotificationsStateUpdate = */ true,
                /* sendEmptyState = */ (SubscriptionMethod_ == ESubscriptionMethod::UNSUBSCRIBE),
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

                    HttpReply_ = TReply(GetReplyDataForManageRequest(), THttpHeaders(), 200);
                }
            );
        }
    );
}

TString TSubscriptionsDevicesHttpRequest::GetReplyDataForGetRequest(
    const TVector<TString>& userUnsubscribedDevices,
    const NAlice::TIoTUserInfo& iotUserInfo
) const {
    const auto& subscriptionsInfo = GetSubscriptionsInfo();

    NJson::TJsonMap response;

    THashSet<TStringBuf> userUnsubscribedDevicesSet = THashSet<TStringBuf>(
        userUnsubscribedDevices.begin(),
        userUnsubscribedDevices.end()
    );

    response["code"] = 200;
    {
        auto& payload = response["payload"] = NJson::TJsonMap();

        {
            auto& rooms = payload["rooms"] = NJson::TJsonArray();

            for (const auto& room : iotUserInfo.GetRooms()) {
                NJson::TJsonMap roomMap;

                roomMap["id"] = room.GetId();
                roomMap["name"] = room.GetName();
                roomMap["household_id"] = room.GetHouseholdId();

                rooms.AppendValue(roomMap);
            }
        }

        {
            auto& devices = payload["devices"] = NJson::TJsonArray();

            for (const auto& device : iotUserInfo.GetDevices()) {
                if (!subscriptionsInfo.IsUserDeviceTypeSuitableForSubscriptions(device.GetType())) {
                    continue;
                }

                NJson::TJsonMap deviceMap;

                deviceMap["id"] = device.GetId();
                deviceMap["name"] = device.GetName();
                deviceMap["room_id"] = device.GetRoomId();
                deviceMap["subscribed"] = !userUnsubscribedDevicesSet.contains(device.GetQuasarInfo().GetDeviceId());

                // As is from python https://a.yandex-team.ru/svn/trunk/arcadia/alice/uniproxy/library/notificator/subscribes.py?rev=r9691064#L286
                // Do not use GetType here, some services really use data for analytics in their logic
                // For example: https://a.yandex-team.ru/arcadia/alice/alice4business/alice-in-business-api/src/services/bulbasaur/device-list.ts?rev=r9737755#L36)
                deviceMap["type"] = device.GetAnalyticsType();

                {
                    auto& quasarInfo = deviceMap["quasar_info"] = NJson::TJsonMap();

                    quasarInfo["device_id"] = device.GetQuasarInfo().GetDeviceId();
                    quasarInfo["platform"] = device.GetQuasarInfo().GetPlatform();
                }

                devices.AppendValue(deviceMap);
            }
        }
    }

    return NJson::WriteJson(response, false, false, false);
}

TString TSubscriptionsDevicesHttpRequest::GetReplyDataForManageRequest() {
    return TString::Join(
        R"({"code": 200, "payload": {"status": ")",
        SubscriptionMethod_ == ESubscriptionMethod::SUBSCRIBE
            ? "subscribed"
            : "unsubscribed",
        R"(" }})"
    );
}

} // namespace NMatrix::NNotificator
