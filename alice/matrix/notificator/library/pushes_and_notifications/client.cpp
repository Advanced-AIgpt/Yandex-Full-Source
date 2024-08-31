#include "client.h"

#include <alice/matrix/notificator/library/subscriptions_info/subscriptions_info.h>
#include <alice/matrix/notificator/library/utils/utils.h>

#include <alice/library/proto/protobuf.h>


namespace NMatrix::NNotificator {

namespace {

static constexpr TStringBuf PUSHES_AND_NOTIFICATIONS_CLIENT = "pushes_and_notifications_client";

TExpected<NAlice::NSpeechKit::TDirective, TString> GetNofifySkDirectiveForDevice(
    const TVector<TNotificationsStorage::TNotification>& notifications,
    const ui64 notificationStateVersionId,
    NAlice::NScenarios::TNotifyDirective::ERingType ringType,
    const TString& deviceId,
    const TString& deviceModel
) {
    // TODO(ZION-147) Do it in a better way
    NAlice::NScenarios::TNotifyDirective notifyDirective;
    notifyDirective.SetRing(ringType);
    notifyDirective.SetVersionId(ToString(notificationStateVersionId));

    for (const auto& notification : notifications) {
        if (notification.DeviceId.Defined() && *notification.DeviceId != deviceId) {
            continue;
        }

        auto subscriptionIdRes = TryParseFromString(notification.Notification.GetSubscriptionId(), "subscription id");
        if (!subscriptionIdRes) {
            return subscriptionIdRes.Error();
        }
        if (!GetSubscriptionsInfo().IsDeviceModelSuitableForSubscription(subscriptionIdRes.Success(), deviceModel)) {
            continue;
        }

        auto* newNotification = notifyDirective.MutableNotifications()->Add();
        newNotification->SetId(notification.Notification.GetId());
        newNotification->SetText(notification.Notification.GetText());
        newNotification->SetSubscriptionId(notification.Notification.GetSubscriptionId());
    }

    NAlice::NSpeechKit::TDirective skDirective;
    skDirective.SetName("notify");
    skDirective.MutablePayload()->CopyFrom(NAlice::MessageToStruct(notifyDirective));

    // Some legacy magic https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/system.py?rev=r9029096#L537
    // Do not touch
    if (skDirective.GetPayload().fields().contains("name")) {
        skDirective.MutablePayload()->mutable_fields()->erase("name");
    }

    return skDirective;
}

} // namespace

TPushesAndNotificationsClient::TPushesAndNotificationsClient(
    const TPushesAndNotificationsClientSettings& pushesAndNotificationsClientConfig,
    const TSubwayClientSettings& subwayClientConfig,
    const NYdb::TDriver& driver
)
    : ConnectionsStorage_(
        driver,
        pushesAndNotificationsClientConfig.GetYDBClient()
    )
    , DirectivesStorage_(
        driver,
        pushesAndNotificationsClientConfig.GetYDBClient()
    )
    , LocatorStorage_(
        driver,
        pushesAndNotificationsClientConfig.GetYDBClient()
    )
    , NotificationsStorage_(
        driver,
        pushesAndNotificationsClientConfig.GetYDBClient()
    )
    , SubscriptionsStorage_(
        driver,
        pushesAndNotificationsClientConfig.GetYDBClient()
    )
    , SubwayClient_(subwayClientConfig)
    , MockMode_(pushesAndNotificationsClientConfig.GetMockMode())
    , UseOldConnectionsStorage_(pushesAndNotificationsClientConfig.GetUseOldConnectionsStorage())
{}

NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> TPushesAndNotificationsClient::ListConnections(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    if (UseOldConnectionsStorage_) {
        return LocatorStorage_.List(
            puid,
            deviceId.GetOrElse(""),
            logContext,
            metrics
        );
    } else {
        return ConnectionsStorage_.ListConnections(
            puid,
            deviceId,
            logContext,
            metrics
        );
    }
}

NThreading::TFuture<TExpected<TMap<ui64, TSubscriptionsStorage::TUserSubscription>, TString>> TPushesAndNotificationsClient::GetUserSubscriptionsForUser(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.GetUserSubscriptionsForUser(
        puid,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<TVector<TString>, TString>> TPushesAndNotificationsClient::GetUserUnsubscribedDevices(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.GetUserUnsubscribedDevices(
        puid,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserSubscriptionsInfo, TString>> TPushesAndNotificationsClient::GetUserSubscriptionsInfo(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    auto userSubscriptionsFut = SubscriptionsStorage_.GetUserSubscriptionsForUser(puid, logContext, metrics);
    auto userUnsubscribedDevicesFut = SubscriptionsStorage_.GetUserUnsubscribedDevices(puid, logContext, metrics);

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            userSubscriptionsFut.Apply([](const auto&){}),
            userUnsubscribedDevicesFut.Apply([](const auto&){}),
        })
    ).Apply(
        [userSubscriptionsFut, userUnsubscribedDevicesFut] (const auto&) -> TExpected<TUserSubscriptionsInfo, TString> {
            auto userSubscriptionsRes = userSubscriptionsFut.GetValueSync();
            auto userUnsubscribedDevicesRes = userUnsubscribedDevicesFut.GetValueSync();

            if (!userSubscriptionsRes) {
                return userSubscriptionsRes.Error();
            }
            if (!userUnsubscribedDevicesRes) {
                return userUnsubscribedDevicesRes.Error();
            }

            return TUserSubscriptionsInfo({
                .SubscriptionIdToUserSubscriptionInfo = userSubscriptionsRes.Success(),
                .UnsubscribedDevices = userUnsubscribedDevicesRes.Success(),
            });
        }
    );
}

NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::SubscribeUser(
    const TString& puid,
    const ui64 subscriptionId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.SubscribeUser(
        puid,
        subscriptionId,
        logContext,
        metrics
    );
}
NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::UnsubscribeUser(
    const TString& puid,
    const ui64 subscriptionId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.UnsubscribeUser(
        puid,
        subscriptionId,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::SubscribeUserDevice(
    const TString& puid,
    const TString& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.SubscribeUserDevice(
        puid,
        deviceId,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::UnsubscribeUserDevice(
    const TString& puid,
    const TString& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.UnsubscribeUserDevice(
        puid,
        deviceId,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<TVector<TSubscriptionsStorage::TUserSubscription>, TString>> TPushesAndNotificationsClient::GetUserSubscriptionsBySubscriptionId(
    const ui64 subscriptionId,
    const TMaybe<ui64> afterTimestamp,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return SubscriptionsStorage_.GetUserSubscriptionsBySubscriptionId(
        subscriptionId,
        afterTimestamp,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<TVector<TNotificationsStorage::TNotification>, TString>> TPushesAndNotificationsClient::GetNotifications(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    const bool fromArchive,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return NotificationsStorage_.GetNotifications(
        puid,
        deviceId,
        fromArchive,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<ui64, TString>> TPushesAndNotificationsClient::GetArchivedNotificationsCount(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return NotificationsStorage_.GetArchivedNotificationsCount(
        puid,
        deviceId,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString>> TPushesAndNotificationsClient::ActualizeAndGetUserNotificationsInfo(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    const bool forceNotificationsStateUpdate,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    auto userNotificationsStateFut = NotificationsStorage_.GetUserNotificationsState(puid, logContext, metrics);
    auto notificationsFut = NotificationsStorage_.GetNotifications(puid, deviceId, false, logContext, metrics);

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            userNotificationsStateFut.Apply([](const auto&){}),
            notificationsFut.Apply([](const auto&){}),
        })
    ).Apply(
        [this, puid, forceNotificationsStateUpdate, logContext, &metrics, userNotificationsStateFut, notificationsFut] (const auto&) mutable {
            auto userNotificationsStateRes = userNotificationsStateFut.GetValueSync();
            auto notificationsRes = notificationsFut.GetValueSync();

            if (!userNotificationsStateRes) {
                return NThreading::MakeFuture<TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString>>(userNotificationsStateRes.Error());
            }
            if (!notificationsRes) {
                return NThreading::MakeFuture<TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString>>(notificationsRes.Error());
            }

            auto userNotificationsInfo = TUserNotificationsInfo({
                .UserNotificationsState = userNotificationsStateRes.Success(),
                .Notifications = notificationsRes.Success(),
            });

            TMaybe<ui64> minTimestamp;
            for (const auto& notification : userNotificationsInfo.Notifications) {
                auto notificationTimestampRes = TryParseFromString(notification.Notification.GetTimestamp(), "timestamp");
                if (!notificationTimestampRes) {
                    return NThreading::MakeFuture<TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString>>(notificationTimestampRes.Error());
                }
                const auto notificationTimestamp = notificationTimestampRes.Success();
                if (!minTimestamp || *minTimestamp > notificationTimestamp) {
                    minTimestamp = notificationTimestamp;
                }
            }

            if (
                forceNotificationsStateUpdate ||
                (userNotificationsInfo.UserNotificationsState.VersionId != 0 && userNotificationsInfo.UserNotificationsState.LastTimestamp != minTimestamp.GetOrElse(0))
            ) {
                return NotificationsStorage_.UpdateUserNotificationsState(
                    puid,
                    minTimestamp.GetOrElse(0),
                    logContext,
                    metrics
                ).Apply(
                    [userNotificationsInfo = std::move(userNotificationsInfo)](
                        const NThreading::TFuture<TExpected<TNotificationsStorage::TUserNotificationsState, TString>>& updateUserNotificationsStateFut
                    ) mutable -> TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString> {
                        auto updateUserNotificationsStateRes = updateUserNotificationsStateFut.GetValueSync();
                        if (!updateUserNotificationsStateRes) {
                            return updateUserNotificationsStateRes.Error();
                        }

                        userNotificationsInfo.UserNotificationsState = updateUserNotificationsStateRes.Success();
                        return userNotificationsInfo;
                    }
                );
            } else {
                return NThreading::MakeFuture<TExpected<TPushesAndNotificationsClient::TUserNotificationsInfo, TString>>(userNotificationsInfo);
            }
        }
    );
}

NThreading::TFuture<TExpected<TNotificationsStorage::EAddNotificationResult, TString>> TPushesAndNotificationsClient::AddNotification(
    const TString& puid,
    const TNotificationsStorage::TNotification& notification,
    const bool allowNotificationDuplicates,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return NotificationsStorage_.AddNotification(
        puid,
        notification,
        allowNotificationDuplicates,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::MarkNotificationsAsRead(
    const TString& puid,
    const TVector<TString>& notificationIds,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return NotificationsStorage_.MarkNotificationsAsRead(
        puid,
        notificationIds,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<TVector<TDirectivesStorage::TDirective>, TString>> TPushesAndNotificationsClient::GetDirectives(
    const TDirectivesStorage::TUserDevice& userDevice,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return DirectivesStorage_.GetDirectives(
        userDevice,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::AddDirective(
    const TDirectivesStorage::TUserDevice& userDevice,
    const TDirectivesStorage::TDirective& directive,
    const TDuration ttl,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    return DirectivesStorage_.AddDirective(
        userDevice,
        directive,
        ttl,
        logContext,
        metrics
    );
}

NThreading::TFuture<TExpected<void, TString>> TPushesAndNotificationsClient::RemoveAllUserData(
    const TString& puid,
    TLogContext logContext,
    TSourceMetrics& metrics
) {
    auto removeDirectivesDataFut = DirectivesStorage_.RemoveAllUserData(puid, logContext, metrics);
    auto removeNotificationsDataFut = NotificationsStorage_.RemoveAllUserData(puid, logContext, metrics);
    auto removeSubscriptionsDataFut = SubscriptionsStorage_.RemoveAllUserData(puid, logContext, metrics);

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            removeDirectivesDataFut.Apply([](const auto&){}),
            removeNotificationsDataFut.Apply([](const auto&){}),
            removeSubscriptionsDataFut.Apply([](const auto&){}),
        })
    ).Apply(
        [removeDirectivesDataFut, removeNotificationsDataFut, removeSubscriptionsDataFut](const auto&) -> TExpected<void, TString> {
            auto removeDirectivesDataRes = removeDirectivesDataFut.GetValueSync();
            auto removeNotificationsDataRes = removeNotificationsDataFut.GetValueSync();
            auto removeSubscriptionsDataRes = removeSubscriptionsDataFut.GetValueSync();

            if (!removeDirectivesDataRes) {
                return removeDirectivesDataRes.Error();
            }
            if (!removeNotificationsDataRes) {
                return removeNotificationsDataRes.Error();
            }
            if (!removeSubscriptionsDataRes) {
                return removeSubscriptionsDataRes.Error();
            }

            return TExpected<void, TString>::DefaultSuccess();
        }
    );
}

NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>> TPushesAndNotificationsClient::ActualizeUserNotificationsInfoAndSendItToDevices(
    const TString& puid,
    const TMaybe<TString>& deviceId,
    const bool forceNotificationsStateUpdate,
    const bool sendEmptyState,
    const NAlice::NScenarios::TNotifyDirective::ERingType ringType,
    const TMaybe<TPushesAndNotificationsClient::TListConnectionsFilter>& listConnectionsFilter,
    TLogContext& logContext,
    TSourceMetrics& metrics
) {
    auto userNotificationsInfoFut = ActualizeAndGetUserNotificationsInfo(
        puid,
        deviceId,
        forceNotificationsStateUpdate,
        logContext,
        metrics
    );
    auto listConnectionsFut = ListConnections(
        puid,
        deviceId,
        logContext,
        metrics
    );

    return NThreading::WaitAll(
        TVector<NThreading::TFuture<void>>({
            userNotificationsInfoFut.Apply([](const auto&){}),
            listConnectionsFut.Apply([](const auto&){}),
        })
    ).Apply(
        [
            this,
            userNotificationsInfoFut,
            listConnectionsFut,

            puid,
            sendEmptyState,
            ringType,
            listConnectionsFilter,
            logContext,
            &metrics
        ] (const auto&) mutable -> NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>> {
            auto userNotificationsInfoRes = userNotificationsInfoFut.GetValueSync();
            auto listConnectionsRes = listConnectionsFut.GetValueSync();

            if (!userNotificationsInfoRes) {
                return NThreading::MakeFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>(userNotificationsInfoRes.Error());
            }
            if (!listConnectionsRes) {
                return NThreading::MakeFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>(listConnectionsRes.Error());
            }

            auto finalListConnectionsResult = listConnectionsFilter.Defined()
                ? listConnectionsFilter.GetRef()(listConnectionsRes.Success())
                : listConnectionsRes.Success()
            ;

            if (finalListConnectionsResult.Records.empty()) {
                return NThreading::MakeFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>(TExpected<TVector<NUniproxy::TSubwayResponse>, TString>::DefaultSuccess());
            }

            auto& userNotificationsInfo = userNotificationsInfoRes.Success();
            if (sendEmptyState) {
                userNotificationsInfo.Notifications.clear();
            }

            return SendSubwayMessageToAllDevices(
                puid,
                ringType,
                userNotificationsInfo,
                /* directives = */ {},
                finalListConnectionsResult,
                logContext,
                metrics
            );
        }
    );
}

NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>> TPushesAndNotificationsClient::SendSubwayMessageToAllDevices(
    const TString& puid,
    NAlice::NScenarios::TNotifyDirective::ERingType ringType,
    const TMaybe<TUserNotificationsInfo>& userNotificationsInfo,
    const TVector<TDirectivesStorage::TDirective>& directives,
    const TConnectionsStorage::TListConnectionsResult& devices,
    TLogContext& logContext,
    TSourceMetrics& metrics
) {
    if (userNotificationsInfo.Defined()) {
        metrics.PushRate(userNotificationsInfo->Notifications.size(), "notifications_count", PUSHES_AND_NOTIFICATIONS_CLIENT);
    }
    metrics.PushRate(directives.size(), "directives_count", PUSHES_AND_NOTIFICATIONS_CLIENT);

    for (const auto& connectionRecord : devices.Records) {
        logContext.LogEventInfoCombo<NEvClass::TMatrixNotificatorUserDeviceLocation>(
            connectionRecord.UserDeviceInfo.DeviceId,
            connectionRecord.UserDeviceInfo.DeviceInfo.GetDeviceModel(),
            connectionRecord.Endpoint.Ip,
            connectionRecord.Endpoint.Port
        );
    }

    TVector<NUniproxy::TSubwayMessage> subwayMessages;
    subwayMessages.reserve(devices.Records.size());
    for (const auto& connectionRecord : devices.Records) {
        const auto& deviceId = connectionRecord.UserDeviceInfo.DeviceId;

        NUniproxy::TSubwayMessage subwayMessage;
        auto& quasarMessage = *subwayMessage.MutableQuasarMsg();

        quasarMessage.SetUid(puid);
        quasarMessage.SetDeviceId(deviceId);
        quasarMessage.SetStartTime(TInstant::Now().MilliSeconds());

        if (userNotificationsInfo.Defined()) {
            auto notifySkDirective = GetNofifySkDirectiveForDevice(
                userNotificationsInfo->Notifications,
                userNotificationsInfo->UserNotificationsState.VersionId,
                ringType,
                connectionRecord.UserDeviceInfo.DeviceId,
                connectionRecord.UserDeviceInfo.DeviceInfo.GetDeviceModel()
            );

            if (notifySkDirective.IsSuccess()) {
                quasarMessage.MutableSkDirectives()->Add()->CopyFrom(notifySkDirective.Success());
            } else {
                return NThreading::MakeFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>(notifySkDirective.Error());
            }
        }
        for (const auto& directive : directives) {
            *quasarMessage.AddPushIds() = directive.PushId;
            quasarMessage.AddSkDirectives()->CopyFrom(directive.SpeechKitDirective);
        }

        subwayMessage.AddDestinations()->SetDeviceId(deviceId);

        // Do not initiate requests to subway here
        // We may not wait for answers if one of GetNofifySkDirectiveForDevice fails
        // and this will lead to coredump
        subwayMessages.emplace_back(std::move(subwayMessage));
    }

    if (Y_UNLIKELY(MockMode_)) {
        for (const auto& subwayMessage : subwayMessages) {
            logContext.LogEventInfoCombo<NEvClass::TMatrixNotificatorMockSubwayRequest>(subwayMessage);
        }

        return NThreading::MakeFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>>(
            TVector<NUniproxy::TSubwayResponse>(subwayMessages.size(), NUniproxy::TSubwayResponse())
        );
    }

    TVector<NThreading::TFuture<TExpected<NUniproxy::TSubwayResponse, TString>>> futures;
    futures.reserve(devices.Records.size());
    for (size_t i = 0; i < subwayMessages.size(); ++i) {
        // All logs in subway client
        futures.emplace_back(
            SubwayClient_.SendSubwayMessage(
                subwayMessages[i],
                puid,
                devices.Records[i].Endpoint.Ip,
                devices.Records[i].Endpoint.Port,
                logContext,
                metrics
            )
        );
    }

    TVector<NThreading::TFuture<void>> futuresForWait;
    futuresForWait.reserve(futures.size());
    for (auto& fut : futures) {
        futuresForWait.emplace_back(fut.Apply([](const auto&){}));
    }
    return NThreading::WaitAll(futuresForWait).Apply(
        [futures = std::move(futures)](const auto&) mutable -> TExpected<TVector<NUniproxy::TSubwayResponse>, TString> {
            TVector<NUniproxy::TSubwayResponse> result;
            result.reserve(futures.size());
            for (const auto& fut : futures) {
                auto res = fut.GetValueSync();
                if (!res) {
                    return res.Error();
                }

                result.push_back(res.Success());
            }

            return result;
        }
    );
}

bool TPushesAndNotificationsClient::TUserSubscriptionsInfo::IsUserSubscribedToSubscription(ui64 subscriptionId) const {
    return SubscriptionIdToUserSubscriptionInfo.contains(subscriptionId);
}

bool TPushesAndNotificationsClient::TUserSubscriptionsInfo::IsDeviceSubscribed(const TString& deviceId) const {
    return Find(UnsubscribedDevices.begin(), UnsubscribedDevices.end(), deviceId) == UnsubscribedDevices.end();
}

} // namespace NMatrix::NNotificator
