#pragma once

#include <alice/hollywood/library/framework/core/request.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/protos/common/environment_state.pb.h>

namespace NAlice::NHollywood {

class TEnvironmentStateHelper {
public:
    explicit TEnvironmentStateHelper(const TScenarioRunRequestWrapper& request);
    explicit TEnvironmentStateHelper(const NHollywoodFw::TRunRequest& request);

    bool IsDeviceTandemFollower() const;
    bool IsDeviceTandemWithRole(const TEnvironmentGroupInfo_EEnvironmentGroupDeviceRole& role) const;
    TTandemDeviceState GetTandemFollowerState() const;
    TClientInfoProto GetTandemFollowerApplication() const;
    bool IsTandemEnabledForFollower() const;
    TMaybe<TClientInfoProto::TMediaDeviceIdentifier> FindMediaDeviceIdentifier() const;

private:
    const TStringBuf DeviceId_;
    const NScenarios::TDataSource* TandemState_;
};

} // namespace NAlice::NHollywood
