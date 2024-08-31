#include "subscriptions_info.h"

#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/algorithm.h>


namespace NMatrix::NNotificator {


namespace {

const TString& GetRealDeviceModel(const TString& deviceModel) {
    // Hack from old python code
    static const TString correctYandexStationDeviceModel = "yandexstation";
    return deviceModel == "station" ? correctYandexStationDeviceModel : deviceModel;
}

TSubscriptionsConfig GetSubscriptionsConfig() {
    NProtobufJson::TJson2ProtoConfig json2ProtoConfig;
    json2ProtoConfig.AllowUnknownFields = false;
    json2ProtoConfig.UseJsonName = true;
    return NProtobufJson::Json2Proto<TSubscriptionsConfig>(NResource::Find("/subscriptions.json"), json2ProtoConfig);
}

THashSet<NAlice::EUserDeviceType> GetUserDeviceTypesSuitableForSubscriptionsSet(const TSubscriptionsConfig& config) {
    THashSet<NAlice::EUserDeviceType> result;

    for (const auto& userDeviceType : config.GetUserDeviceTypesSuitableForSubscriptions()) {
        result.insert(static_cast<NAlice::EUserDeviceType>(userDeviceType));
    }

    return result;
}

THashMap<ui64, TSubscription> GetSubscriptionsMap(const TSubscriptionsConfig& config) {
    THashMap<ui64, TSubscription> result;

    for (const auto& subscription : config.GetSubscriptions()) {
        Y_ENSURE(
            result.emplace(subscription.GetId(), subscription).second,
            "More than one subscription with same id found"
        );
    }

    return result;
}

THashMap<ui64, THashSet<TString>> GetSubscriptionDeviceModelsMap(const TSubscriptionsConfig& config) {
    THashMap<ui64, THashSet<TString>> result;

    for (const auto& subscription : config.GetSubscriptions()) {
        Y_ENSURE(
            result.emplace(
                subscription.GetId(),
                THashSet<TString>(
                    subscription.GetSettings().GetDeviceModels().begin(),
                    subscription.GetSettings().GetDeviceModels().end()
                )
            ).second,
            "More than one subscription with same id found"
        );
    }

    return result;
}

} // namespace

TSubscriptionsInfo::TSubscriptionsInfo()
    : Config_(GetSubscriptionsConfig())
    , Categories_(Config_.GetCategories().begin(), Config_.GetCategories().end())
    , UserDeviceTypesSuitableForSubscriptions_(GetUserDeviceTypesSuitableForSubscriptionsSet(Config_))
    , SubscriptionsById_(GetSubscriptionsMap(Config_))
    , SubscriptionDeviceModelsById_(GetSubscriptionDeviceModelsMap(Config_))
{}

const TVector<TSubscriptionCategory>& TSubscriptionsInfo::GetCategories() const {
    return Categories_;
}

const THashMap<ui64, TSubscription>& TSubscriptionsInfo::GetSubscriptions() const {
    return SubscriptionsById_;
}

bool TSubscriptionsInfo::HasSubscription(ui64 subscriptionId) const {
    return SubscriptionsById_.contains(subscriptionId);
}

bool TSubscriptionsInfo::IsUserDeviceTypeSuitableForSubscriptions(NAlice::EUserDeviceType userDeviceType) const {
    return UserDeviceTypesSuitableForSubscriptions_.contains(userDeviceType);
}

bool TSubscriptionsInfo::IsDeviceModelSuitableForSubscription(
    const ui64 subscriptionId,
    const TString& deviceModel
) const {
    if (!HasSubscription(subscriptionId)) {
        // There is no subscription with such subscription id, so return false
        return false;
    }

    const auto& subscriptionDeviceModels = SubscriptionDeviceModelsById_.at(subscriptionId);
    if (subscriptionDeviceModels.empty() || deviceModel.empty()) {
        return true;
    }

    return subscriptionDeviceModels.contains(GetRealDeviceModel(deviceModel));
}

TConnectionsStorage::TListConnectionsResult TSubscriptionsInfo::FilterListConnectionsResultBySubscriptionAndUnsubscribedDevices(
    const TConnectionsStorage::TListConnectionsResult& listConnectionsResult,
    ui64 subscriptionId,
    const TVector<TString>& unsubscribedDevices
) const {
    if (!HasSubscription(subscriptionId)) {
        // There is no subscription with such subscription id, so return empty vector.
        return {};
    }
    const auto& subscriptionDeviceModels = SubscriptionDeviceModelsById_.at(subscriptionId);

    THashSet<TStringBuf> unsubscribedDevicesSet = THashSet<TStringBuf>(
        unsubscribedDevices.begin(),
        unsubscribedDevices.end()
    );

    TConnectionsStorage::TListConnectionsResult filteredResult;
    for (auto connectionRecord : listConnectionsResult.Records) {
        if (const auto& deviceModel = connectionRecord.UserDeviceInfo.DeviceInfo.GetDeviceModel();
            !deviceModel.empty() &&
            !subscriptionDeviceModels.contains(GetRealDeviceModel(deviceModel))
        ) {
            continue;
        }

        if (const auto& deviceId = connectionRecord.UserDeviceInfo.DeviceId; !deviceId.empty() && unsubscribedDevicesSet.contains(deviceId)) {
            continue;
        }

        filteredResult.Records.emplace_back(connectionRecord);
    }

    return filteredResult;
}

const TSubscriptionsInfo& GetSubscriptionsInfo() {
    return *Singleton<TSubscriptionsInfo>();
}

} // namespace NMatrix::NNotificator
