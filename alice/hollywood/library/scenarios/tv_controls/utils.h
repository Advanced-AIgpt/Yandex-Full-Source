#pragma once

#include <alice/hollywood/library/scenarios/tv_controls/proto/tv_controls.pb.h>
#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/environment_state/environment_state.h>
#include <alice/hollywood/library/framework/core/request.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/tandem_state.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/protos/endpoint/capabilities/screensaver/capability.pb.h>

namespace NAlice::NHollywoodFw::NTvControls {

    inline static constexpr TStringBuf NLG_NAME = "tv_controls";

    inline bool IsTvOrModuleOrTandemRequest(const TRunRequest& request, const TDeviceState& deviceState) {
        return request.Client().GetClientInfo().IsTvDevice() || request.Client().GetClientInfo().IsYaModule() || (deviceState.HasTandemState() && deviceState.GetTandemState().GetConnected());
    }

    inline TString GetDeviceIdOfTvDevice(const TRunRequest& request, const TDeviceState& deviceState) {
        if (deviceState.GetTandemState().GetConnected()) {
            auto envHelper = NHollywood::TEnvironmentStateHelper(request);
            return envHelper.GetTandemFollowerApplication().GetDeviceId();
        }

        return request.Client().GetClientInfo().DeviceId;
    }

    inline bool SupportsScreensaver(const TRunRequest& request, const NAlice::TDeviceState& deviceState) {
        if (const NScenarios::TDataSource* dataSource = request.GetDataSource(EDataSourceType::ENVIRONMENT_STATE)) {
            const NAlice::TEnvironmentState* envState = &dataSource->GetEnvironmentState();

            const NAlice::TEndpoint* endpoint = NHollywood::FindEndpoint(*envState, GetDeviceIdOfTvDevice(request, deviceState));
            if (!endpoint) {
                return false;
            }

            TScreensaverCapability capability;
            return NHollywood::ParseTypedCapability(capability, *endpoint);
        }

        Y_UNUSED(request);
        return false;
    }

}