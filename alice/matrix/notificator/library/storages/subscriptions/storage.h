#pragma once

#include <alice/matrix/library/ydb/storage.h>

namespace NMatrix::NNotificator {

class TSubscriptionsStorage: public IYDBStorage {
public:
    struct TUserSubscription {
        TString Puid;
        ui64 SubscribedAtTimestamp;
    };

public:
    TSubscriptionsStorage(
        const NYdb::TDriver& driver,
        const TYDBClientSettings& config
    );

    NThreading::TFuture<TExpected<TMap<ui64, TUserSubscription>, TString>> GetUserSubscriptionsForUser(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TVector<TUserSubscription>, TString>> GetUserSubscriptionsBySubscriptionId(
        const ui64 subscriptionId,
        const TMaybe<ui64> afterTimestamp,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

    NThreading::TFuture<TExpected<TVector<TString>, TString>> GetUserUnsubscribedDevices(
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

    NThreading::TFuture<TExpected<void, TString>> RemoveAllUserData(
        const TString& puid,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    static inline constexpr TStringBuf NAME = "subscriptions";
};

} // namespace NMatrix::NNotificator
