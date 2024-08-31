#pragma once

#include <alice/matrix/notificator/library/config/config.pb.h>

#include <alice/matrix/notificator/library/storages/connections/storage.h>
#include <alice/matrix/notificator/library/storages/directives/storage.h>
#include <alice/matrix/notificator/library/storages/locator/storage.h>
#include <alice/matrix/notificator/library/storages/notifications/storage.h>
#include <alice/matrix/notificator/library/storages/subscriptions/storage.h>

#include <alice/matrix/library/clients/subway_client/subway_client.h>
#include <alice/matrix/library/config/config.pb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>


namespace NMatrix::NNotificator {

class TPushesAndNotificationsClient : public TNonCopyable {
public:
    struct TUserSubscriptionsInfo {
        TMap<ui64, TSubscriptionsStorage::TUserSubscription> SubscriptionIdToUserSubscriptionInfo;
        TVector<TString> UnsubscribedDevices;

        bool IsUserSubscribedToSubscription(ui64 subscriptionId) const;
        bool IsDeviceSubscribed(const TString& deviceId) const;
    };

    struct TUserNotificationsInfo {
        TNotificationsStorage::TUserNotificationsState UserNotificationsState;
        TVector<TNotificationsStorage::TNotification> Notifications;
    };

    using TListConnectionsFilter = std::function<TConnectionsStorage::TListConnectionsResult(const TConnectionsStorage::TListConnectionsResult& connections)>;

public:
    TPushesAndNotificationsClient(
        const TPushesAndNotificationsClientSettings& pushesAndNotificationsClientConfig,
        const TSubwayClientSettings& subwayClientConfig,
        const NYdb::TDriver& driver
    );

    // Connections
    NThreading::TFuture<TExpected<TConnectionsStorage::TListConnectionsResult, TString>> ListConnections(
        const TString& puid,
        const TMaybe<TString>& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    // Quasar notifications
    NThreading::TFuture<TExpected<TMap<ui64, TSubscriptionsStorage::TUserSubscription>, TString>> GetUserSubscriptionsForUser(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TVector<TString>, TString>> GetUserUnsubscribedDevices(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TUserSubscriptionsInfo, TString>> GetUserSubscriptionsInfo(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> SubscribeUser(
        const TString& puid,
        const ui64 subscriptionId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> UnsubscribeUser(
        const TString& puid,
        const ui64 subscriptionId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> SubscribeUserDevice(
        const TString& puid,
        const TString& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> UnsubscribeUserDevice(
        const TString& puid,
        const TString& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TVector<TSubscriptionsStorage::TUserSubscription>, TString>> GetUserSubscriptionsBySubscriptionId(
        const ui64 subscriptionId,
        const TMaybe<ui64> afterTimestamp,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TVector<TNotificationsStorage::TNotification>, TString>> GetNotifications(
        const TString& puid,
        const TMaybe<TString>& deviceId,
        const bool fromArchive,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<ui64, TString>> GetArchivedNotificationsCount(
        const TString& puid,
        const TMaybe<TString>& deviceId,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TUserNotificationsInfo, TString>> ActualizeAndGetUserNotificationsInfo(
        const TString& puid,
        const TMaybe<TString>& deviceId,
        const bool forceNotificationsStateUpdate,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TNotificationsStorage::EAddNotificationResult, TString>> AddNotification(
        const TString& puid,
        const TNotificationsStorage::TNotification& notification,
        const bool allowNotificationDuplicates,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> MarkNotificationsAsRead(
        const TString& puid,
        const TVector<TString>& notificationIds,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    // Technical pushes
    NThreading::TFuture<TExpected<TVector<TDirectivesStorage::TDirective>, TString>> GetDirectives(
        const TDirectivesStorage::TUserDevice& userDevice,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<void, TString>> AddDirective(
        const TDirectivesStorage::TUserDevice& userDevice,
        const TDirectivesStorage::TDirective& directive,
        const TDuration ttl,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    // GDPR
    NThreading::TFuture<TExpected<void, TString>> RemoveAllUserData(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    // Subway
    NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>> ActualizeUserNotificationsInfoAndSendItToDevices(
        const TString& puid,
        const TMaybe<TString>& deviceId,
        const bool forceNotificationsStateUpdate,
        // Sometimes it is needed to clear notifications state on a certain device/devices
        // For example on device unsubscribe or on GDPR removal
        const bool sendEmptyState,
        const NAlice::NScenarios::TNotifyDirective::ERingType ringType,
        const TMaybe<TListConnectionsFilter>& listConnectionsFilter,
        TLogContext& logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TVector<NUniproxy::TSubwayResponse>, TString>> SendSubwayMessageToAllDevices(
        const TString& puid,
        NAlice::NScenarios::TNotifyDirective::ERingType ringType,
        const TMaybe<TUserNotificationsInfo>& userNotificationsInfo,
        const TVector<TDirectivesStorage::TDirective>& directives,
        const TConnectionsStorage::TListConnectionsResult& devices,
        TLogContext& logContext,
        TSourceMetrics& metrics
    );

private:
    TConnectionsStorage ConnectionsStorage_;
    TDirectivesStorage DirectivesStorage_;
    TLocatorStorage LocatorStorage_;
    TNotificationsStorage NotificationsStorage_;
    TSubscriptionsStorage SubscriptionsStorage_;
    TSubwayClient SubwayClient_;

    const bool MockMode_;
    const bool UseOldConnectionsStorage_;
};

} // namespace NMatrix::NNotificator
