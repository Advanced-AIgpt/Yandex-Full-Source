#pragma once

#include <alice/bass/forms/context/context.h>

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <google/protobuf/util/json_util.h>


namespace NBASS::NVideo {

class TEnvironmentStateHelper {
public:
    explicit TEnvironmentStateHelper(const TContext& ctx)
        : DeviceId_{ctx.GetDeviceId()}
        , EnvStatePtr_{ctx.DataSources().FindPtr(ENVIRONMENT_STATE_TYPE)}
        , TandemPtr_{ctx.DataSources().FindPtr(TANDEM_ENVIRONMENT_STATE_TYPE)}
    {
        if (EnvStatePtr_) {
            google::protobuf::util::JsonStringToMessage((*EnvStatePtr_)["environment_state"].ToJson(), &EnvState_);
        }
        if (TandemPtr_) {
            google::protobuf::util::JsonStringToMessage((*TandemPtr_)["tandem_environment_state"].ToJson(), &TandemEnvState_);
        }
    }

    bool IsDeviceTandemFollower() const;
    bool IsDeviceTandemWithRole(const NAlice::TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole& role) const;
    NAlice::TTandemDeviceState GetTandemFollowerState() const;
    NAlice::TClientInfoProto GetTandemFollowerApplication() const;
    bool IsTandemEnabledForFollower() const;
    TMaybe<NAlice::TClientInfoProto::TMediaDeviceIdentifier> FindMediaDeviceIdentifier() const;

    TMaybe<NAlice::TEnvironmentState> GetEnvironmentState() {
        if (EnvStatePtr_) {
            return EnvState_;
        }
        return Nothing();
    }

private:
    const TString DeviceId_;
    const NSc::TValue* EnvStatePtr_;
    const NSc::TValue* TandemPtr_;

    NAlice::TEnvironmentState EnvState_;
    NAlice::TTandemEnvironmentState TandemEnvState_;
};

} // NVideo::NBASS
