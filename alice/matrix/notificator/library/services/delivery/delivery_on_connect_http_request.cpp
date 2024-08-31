#include "delivery_on_connect_http_request.h"


namespace NMatrix::NNotificator {

namespace {

using std::placeholders::_1;

} // namespace

TDeliveryOnConnectHttpRequest::TDeliveryOnConnectHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    TPushesAndNotificationsClient& pushesAndNotificationsClient
)
    : TProtoHttpRequest(
        NAME,
        requestCounterRef,
        /* needThreadSafeLogFrame = */ true,
        rtLogClient,
        request,
        [](NNeh::NHttp::ERequestType method) {
            return NNeh::NHttp::ERequestType::Post == method;
        }
    )
    , PushesAndNotificationsClient_(pushesAndNotificationsClient)
{}

NThreading::TFuture<void> TDeliveryOnConnectHttpRequest::ServeAsync() {
    return PushesAndNotificationsClient_.GetDirectives(
        TDirectivesStorage::TUserDevice({
            .Puid = Request_->GetPuid(),
            .DeviceId = Request_->GetDeviceId(),
        }),
        LogContext_,
        Metrics_
    ).Apply(
        std::bind(&TDeliveryOnConnectHttpRequest::OnDirectivesResult, this, _1)
    );
}

TDeliveryOnConnectHttpRequest::TReply TDeliveryOnConnectHttpRequest::GetReply() const {
    return TReply(GetReplyData(), THttpHeaders(), 200);
}

TString TDeliveryOnConnectHttpRequest::GetReplyData() const {
    return R"({"code": 200})";
}

TConnectionsStorage::TListConnectionsResult TDeliveryOnConnectHttpRequest::GetListConnectionsResult() const {
    TDeviceInfo deviceInfo;
    deviceInfo.SetDeviceModel(Request_->GetDeviceModel());

    // Fake listConnectionsResult
    // TODO(ZION-80, chegoryu) remove this with delivery/on_connect removal
    return TConnectionsStorage::TListConnectionsResult({
        .Records = {
            {
                .Endpoint = TConnectionsStorage::TEndpoint({
                    // Locator storage uses fqdns, not ips :(
                    // SubwayClient works correctly with fqdn in ip field
                    .Ip = Request_->GetHostname(),
                    // Fake shard id in locator storage
                    .ShardId = 0,
                    .Port = TLocatorStorage::SUBWAY_PORT,
                    // Fake monotonic in locator storage
                    .Monotonic = 0,
                }),
                .UserDeviceInfo = TConnectionsStorage::TUserDeviceInfo({
                    .Puid = "fake",
                    .DeviceId =  Request_->GetDeviceId(),
                    .DeviceInfo = std::move(deviceInfo),
                }),
            },
        },
    });
}

NThreading::TFuture<void> TDeliveryOnConnectHttpRequest::OnDirectivesResult(
    const NThreading::TFuture<TExpected<TVector<TDirectivesStorage::TDirective>, TString>>& directivesFut
) {
    auto directivesRes = directivesFut.GetValueSync();
    if (!directivesRes) {
        SetError(directivesRes.Error(), 500);
        return NThreading::MakeFuture();
    }
    auto directives = std::move(directivesRes.Success());

    return PushesAndNotificationsClient_.GetUserSubscriptionsInfo(
        Request_->GetPuid(),
        LogContext_,
        Metrics_
    ).Apply(
        std::bind(&TDeliveryOnConnectHttpRequest::OnGetUserSubscriptionsInfo, this, _1, directives)
    );
}

NThreading::TFuture<void> TDeliveryOnConnectHttpRequest::OnGetUserSubscriptionsInfo(
    const NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserSubscriptionsInfo, TString>>& userSubscriptionsInfoResultFut,
    TVector<TDirectivesStorage::TDirective> directives
) {
    auto userSubscriptionsInfoResult = userSubscriptionsInfoResultFut.GetValueSync();
    if (!userSubscriptionsInfoResult) {
        SetError(userSubscriptionsInfoResult.Error(), 500);
        return NThreading::MakeFuture();
    }
    const auto& userSubscriptionsInfo = userSubscriptionsInfoResult.Success();

    NThreading::TFuture<TExpected<TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>, TString>> userNotificationsInfoFut;
    if (userSubscriptionsInfo.IsDeviceSubscribed(Request_->GetDeviceId())) {
        userNotificationsInfoFut = PushesAndNotificationsClient_.ActualizeAndGetUserNotificationsInfo(
            Request_->GetPuid(),
            Request_->GetDeviceId().empty() ? Nothing() : TMaybe<TString>(Request_->GetDeviceId()),
            false,
            LogContext_,
            Metrics_
        ).Apply(
            [](
                const NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString>>& userNotificationsInfoFut
            ) -> TExpected<TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>, TString> {
                auto userNotificationsInfoRes = userNotificationsInfoFut.GetValueSync();
                if (!userNotificationsInfoRes) {
                    return userNotificationsInfoRes.Error();
                }
                return TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>(userNotificationsInfoRes.Success());
            }
        );
    } else {
        userNotificationsInfoFut = NThreading::MakeFuture<TExpected<TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>, TString>>(
            TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>(Nothing())
        );
    }

    return userNotificationsInfoFut.Apply(
        std::bind(
            &TDeliveryOnConnectHttpRequest::OnGetUserNotificationsInfo,
            this,
            _1,
            directives
        )
    );
}

NThreading::TFuture<void> TDeliveryOnConnectHttpRequest::OnGetUserNotificationsInfo(
    const NThreading::TFuture<TExpected<TMaybe<TPushesAndNotificationsClient::TUserNotificationsInfo>, TString>>& userNotificationsInfoFut,
    TVector<TDirectivesStorage::TDirective> directives
) {
    auto userNotificationsInfoRes = userNotificationsInfoFut.GetValueSync();
    if (!userNotificationsInfoRes) {
        SetError(userNotificationsInfoRes.Error(), 500);
        return NThreading::MakeFuture();
    }

    return PushesAndNotificationsClient_.SendSubwayMessageToAllDevices(
        Request_->GetPuid(),
        NAlice::NScenarios::TNotifyDirective::NoSound,
        userNotificationsInfoRes.Success(),
        directives,
        GetListConnectionsResult(),
        LogContext_,
        Metrics_
    ).Apply(
        [this](const NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>& fut) mutable {
            const auto& futRes = fut.GetValueSync();
            if (!futRes) {
                SetError(futRes.Error(), 500);
            }
        }
    );
}

} // namespace NMatrix::NNotificator
