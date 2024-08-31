#include "environment_state.h"

#include <alice/megamind/protos/common/tandem_state.pb.h>

using namespace NAlice;
namespace NBASS::NVideo {

bool TandemGroupHasDeviceWithRole(const TEnvironmentGroupInfo& group, const TString id, const TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole role) {
    if (group.GetType() != TEnvironmentGroupInfo_EEnvironmentGroupType_tandem) {
        return false;
    }
    return AnyOf(group.GetDevices(), [id, role](const auto& device) {
        return device.GetId() == id &&
               device.GetRole() == role;
    });
}

bool TEnvironmentStateHelper::IsDeviceTandemFollower() const {
    return IsDeviceTandemWithRole(TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_follower);
}

bool TEnvironmentStateHelper::IsDeviceTandemWithRole(const TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole& role) const {
    if (!TandemPtr_) {
        return false;
    }
    return AnyOf(TandemEnvState_.GetGroups(), [=, id = DeviceId_](const auto& group) {
        return TandemGroupHasDeviceWithRole(group, id, role);
    });
}

TTandemDeviceState TEnvironmentStateHelper::GetTandemFollowerState() const {
    if (!TandemPtr_) {
        return TTandemDeviceState::default_instance();
    }
    const auto& tandemGroup = FindIfPtr(TandemEnvState_.GetGroups(), [id = DeviceId_](const auto& group) {
        return TandemGroupHasDeviceWithRole(group, id, TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_leader);
    });
    if (tandemGroup != nullptr) {
        const auto& tandemFollower = FindIfPtr(tandemGroup->GetDevices(), [](const auto& device) {
            return device.GetRole() == TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_follower;
        });
        if (tandemFollower != nullptr) {
            const auto& tandemFollowerInfo = FindIfPtr(TandemEnvState_.GetDevices(), [id = tandemFollower->GetId()](const auto& device) {

                return device.GetApplication().GetDeviceId() == id;
            });
            return tandemFollowerInfo != nullptr ? tandemFollowerInfo->GetTandemDeviceState() : TTandemDeviceState::default_instance();
        }
    }
    return TTandemDeviceState::default_instance();
}

TClientInfoProto TEnvironmentStateHelper::GetTandemFollowerApplication() const {
    if (!TandemPtr_) {
        return TClientInfoProto::default_instance();
    }
    const auto& tandemGroup = FindIfPtr(TandemEnvState_.GetGroups(), [id = DeviceId_](const auto& group) {
        return TandemGroupHasDeviceWithRole(group, id, TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_leader);
    });
    if (tandemGroup != nullptr) {
        const auto& tandemFollower = FindIfPtr(tandemGroup->GetDevices(), [](const auto& device) {
            return device.GetRole() == TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole_follower;
        });
        if (tandemFollower != nullptr) {
            const auto& tandemFollowerInfo = FindIfPtr(TandemEnvState_.GetDevices(), [id = tandemFollower->GetId()](const auto& device) {
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
    if (!TandemPtr_) {
        return Nothing();
    }

    if (IsTandemEnabledForFollower()) {
        return GetTandemFollowerApplication().GetMediaDeviceIdentifier();
    } else {
        const auto& tandemFollowerInfo = FindIfPtr(TandemEnvState_.GetDevices(), [id = DeviceId_](const auto& device) {
            return device.GetApplication().GetDeviceId() == id;
        });
        if (tandemFollowerInfo && tandemFollowerInfo->GetApplication().HasMediaDeviceIdentifier()) {
            return tandemFollowerInfo->GetApplication().GetMediaDeviceIdentifier();
        }
        return Nothing();
    }
}

}  // namespace NBASS::NVideo
