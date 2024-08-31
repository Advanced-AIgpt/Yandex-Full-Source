#pragma once

#include <alice/matrix/library/ydb/storage.h>

#include <alice/megamind/protos/scenarios/notification_state.pb.h>

namespace NMatrix::NNotificator {

class TNotificationsStorage: public IYDBStorage {
public:
    struct TUserNotificationsState {
        ui64 VersionId;
        ui64 LastTimestamp;
    };

    struct TNotification {
        TMaybe<TString> DeviceId;
        NAlice::TNotification Notification;

        ui64 GetNotificationContentHash() const;
    };

    enum class EAddNotificationResult {
        UNKNOWN = 0,
        ADDED = 1,
        ALREADY_EXIST = 2,
    };

public:
    TNotificationsStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    NThreading::TFuture<TExpected<TUserNotificationsState, TString>> GetUserNotificationsState(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );
    NThreading::TFuture<TExpected<TUserNotificationsState, TString>> UpdateUserNotificationsState(
        const TString& puid,
        const TMaybe<ui64> lastTimestamp,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TVector<TNotification>, TString>> GetNotifications(
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

    NThreading::TFuture<TExpected<EAddNotificationResult, TString>> AddNotification(
        const TString& puid,
        const TNotification& notification,
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

    NThreading::TFuture<TExpected<void, TString>> RemoveAllUserData(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    // Constants as is from python notificator.
    static inline constexpr TDuration ARCHIVED_NOTIFICATION_TTL = TDuration::Days(2);
    static inline constexpr TDuration CURRENT_NOTIFICATION_TTL = TDuration::Days(2);
    static inline constexpr ui32 NOTIFICATIONS_SELECT_LIMIT = 99;

    static inline constexpr TStringBuf NAME = "notifications";
};

} // namespace NMatrix::NNotificator
