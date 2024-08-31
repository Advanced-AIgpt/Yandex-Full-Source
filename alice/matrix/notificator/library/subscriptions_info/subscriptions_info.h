#pragma once

#include <alice/matrix/notificator/library/subscriptions_info/protos/subscriptions_config.pb.h>

#include <alice/matrix/notificator/library/storages/connections/storage.h>

#include <alice/protos/data/device/info.pb.h>

#include <util/generic/singleton.h>


namespace NMatrix::NNotificator {

class TSubscriptionsInfo : public TNonCopyable {
private:
    TSubscriptionsInfo();

public:
    Y_DECLARE_SINGLETON_FRIEND()

    const TVector<TSubscriptionCategory>& GetCategories() const;
    const THashMap<ui64, TSubscription>& GetSubscriptions() const;
    bool HasSubscription(ui64 subscriptionId) const;

    bool IsUserDeviceTypeSuitableForSubscriptions(NAlice::EUserDeviceType userDeviceType) const;

    bool IsDeviceModelSuitableForSubscription(
        const ui64 subscriptionId,
        const TString& deviceModel
    ) const;

    TConnectionsStorage::TListConnectionsResult FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
        const TConnectionsStorage::TListConnectionsResult& listConnectionsResult,
        ui64 subscriptionId,
        const TVector<TString>& unsubscribedDevices
    ) const;


private:
    const TSubscriptionsConfig Config_;
    const TVector<TSubscriptionCategory> Categories_;
    const THashSet<NAlice::EUserDeviceType> UserDeviceTypesSuitableForSubscriptions_;
    const THashMap<ui64, TSubscription> SubscriptionsById_;
    const THashMap<ui64, THashSet<TString>> SubscriptionDeviceModelsById_;
};

const TSubscriptionsInfo& GetSubscriptionsInfo();

} // namespace NMatrix::NNotificator
