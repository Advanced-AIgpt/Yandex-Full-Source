#include "environment_state.h"

#include <alice/megamind/protos/common/tandem_state.pb.h>

namespace NAlice::NHollywood {

namespace {

bool TandemGroupHasDeviceWithRole(const TEnvironmentGroupInfo& group, const TStringBuf id, const TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole role) {
    if (group.GetType() != TEnvironmentGroupInfo_EEnvironmentGroupType_tandem) {
        return false;
    }
    return AnyOf(group.GetDevices(), [id, role](const auto& device) {
        return device.GetId() == id &&
               device.GetRole() == role;
    });
}

} // namespace

TEnvironmentStateHelper::TEnvironmentStateHelper(const TScenarioRunRequestWrapper& request)
    : DeviceId_{request.BaseRequestProto().GetClientInfo().GetDeviceId()}
    , TandemState_{request.GetDataSource(EDataSourceType::TANDEM_ENVIRONMENT_STATE)}
{
}

TEnvironmentStateHelper::TEnvironmentStateHelper(const NHollywoodFw::TRunRequest& request)
    : DeviceId_{request.GetRunRequest().GetBaseRequest().GetClientInfo().GetDeviceId()}
    , TandemState_{request.GetDataSource(EDataSourceType::TANDEM_ENVIRONMENT_STATE)}
{
}

bool TEnvironmentStateHelper::IsDeviceTandemFollower() const {
    return IsDeviceTandemWithRole(TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_follower);
}

bool TEnvironmentStateHelper::IsDeviceTandemWithRole(const TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole& role) const {
    if (!TandemState_) {
        return false;
    }
    const TTandemEnvironmentState& proto = TandemState_->GetTandemEnvironmentState();
    return AnyOf(proto.GetGroups(), [=, id = DeviceId_](const auto& group) {
        return TandemGroupHasDeviceWithRole(group, id, role);
    });
}

TTandemDeviceState TEnvironmentStateHelper::GetTandemFollowerState() const {
    if (!TandemState_) {
        return TTandemDeviceState::default_instance();
    }
    const TTandemEnvironmentState& proto = TandemState_->GetTandemEnvironmentState();
    const auto& tandemGroup = FindIfPtr(proto.GetGroups(), [id = DeviceId_](const auto& group) {
        return TandemGroupHasDeviceWithRole(group, id, TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_leader);
    });
    if (tandemGroup != nullptr) {
        const auto& tandemFollower = FindIfPtr(tandemGroup->GetDevices(), [](const auto& device) {
            return device.GetRole() == TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_follower;
        });
        if (tandemFollower != nullptr) {
            const auto& tandemFollowerInfo = FindIfPtr(proto.GetDevices(), [id = tandemFollower->GetId()](const auto& device) {
                return device.GetApplication().GetDeviceId() == id;
            });
            return tandemFollowerInfo != nullptr ? tandemFollowerInfo->GetTandemDeviceState() : TTandemDeviceState::default_instance();
        }
    }
    return TTandemDeviceState::default_instance();
}

TClientInfoProto TEnvironmentStateHelper::GetTandemFollowerApplication() const {
    if (!TandemState_) {
        return TClientInfoProto::default_instance();
    }
    const TTandemEnvironmentState& proto = TandemState_->GetTandemEnvironmentState();
    const auto& tandemGroup = FindIfPtr(proto.GetGroups(), [id = DeviceId_](const auto& group) {
        return TandemGroupHasDeviceWithRole(group, id, TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_leader);
    });
    if (tandemGroup != nullptr) {
        const auto& tandemFollower = FindIfPtr(tandemGroup->GetDevices(), [](const auto& device) {
            return device.GetRole() == TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_follower;
        });
        if (tandemFollower != nullptr) {
            const auto& tandemFollowerInfo = FindIfPtr(proto.GetDevices(), [id = tandemFollower->GetId()](const auto& device) {
                return device.GetApplication().GetDeviceId() == id;
            });
            return tandemFollowerInfo != nullptr ? tandemFollowerInfo->GetApplication() : TClientInfoProto::default_instance();
        }
    }
    return TClientInfoProto::default_instance();
}

bool TEnvironmentStateHelper::IsTandemEnabledForFollower() const {
    const auto& tandemFollowerState = GetTandemFollowerState();
    return tandemFollowerState.HasTandemState() && tandemFollowerState.GetTandemState().GetConnected();
}

TMaybe<TClientInfoProto::TMediaDeviceIdentifier> TEnvironmentStateHelper::FindMediaDeviceIdentifier() const {
    if (!TandemState_) {
        return Nothing();
    }

    if (IsTandemEnabledForFollower()) {
        return GetTandemFollowerApplication().GetMediaDeviceIdentifier();
    } else {
        const TTandemEnvironmentState& proto = TandemState_->GetTandemEnvironmentState();
        const auto& tandemFollowerInfo = FindIfPtr(proto.GetDevices(), [id = DeviceId_](const auto& device) {
            return device.GetApplication().GetDeviceId() == id;
        });
        if (tandemFollowerInfo->GetApplication().HasMediaDeviceIdentifier()) {
            return tandemFollowerInfo->GetApplication().GetMediaDeviceIdentifier();
        }
        return Nothing();
    }
}

}  // namespace NAlice::NHollywood
