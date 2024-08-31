#include "client_biometry.h"

#include <alice/hollywood/library/environment_state/endpoint.h>

namespace NAlice::NHollywood {

bool DeviceSupportsBiometry(const TEnvironmentState& environmentState, TStringBuf deviceId) {
    const auto* endpoint = FindEndpoint(environmentState, deviceId);
    if (!endpoint) {
        return false;
    }

    TBioCapability capability;
    if (!ParseTypedCapability(capability, *endpoint)) {
        return false;
    }

    return true;
}

bool SupportsClientBiometry(const TScenarioRunRequestWrapper& request) {
    const auto* environmentState = GetEnvironmentStateProto(request);
    return environmentState && DeviceSupportsBiometry(*environmentState, request.ClientInfo().DeviceId);
}

bool SupportsClientBiometry(const NAlice::NHollywoodFw::TRunRequest& request) {
    const auto* ds = request.GetDataSource(EDataSourceType::ENVIRONMENT_STATE);
    return ds && DeviceSupportsBiometry(ds->GetEnvironmentState(), request.Client().GetClientInfo().DeviceId);
}

bool ValidateGuestOptionsDataSource(TRTLogger& logger, const NAlice::TGuestOptions& guestOptions) {
    if (!guestOptions.HasIsOwnerEnrolled()) {
        LOG_ERROR(logger) << "GuestOptions data source is present, but has't IsOwnerEnrolled field";
        return false;
    }
    if (guestOptions.GetStatus() == TGuestOptions::NoMatch) {
        return true;
    }
    if (!guestOptions.HasYandexUID() || guestOptions.GetYandexUID().Empty()) {
        LOG_WARN(logger) << "GuestOptions data source is present, but has empty YandexUID";
        return false;
    }
    if (!guestOptions.HasOAuthToken() || guestOptions.GetOAuthToken().Empty()) {
        LOG_WARN(logger) << "GuestOptions data source is present, but has empty OAuthToken";
        return false;
    }
    if (!guestOptions.HasPersId() || guestOptions.GetPersId().Empty()) {
        LOG_WARN(logger) << "GuestOptions data source is present, but has empty PersId";
        return false;
    }
    return true;
}

} // namespace NAlice::NHollywood
