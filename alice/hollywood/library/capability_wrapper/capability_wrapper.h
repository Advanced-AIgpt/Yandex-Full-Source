#pragma once

#include <alice/hollywood/library/biometry/client_biometry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/s3_animations/s3_animations.h>

namespace NAlice::NHollywood {

template <class TScenarioRequestWrapper>
class TCapabilityWrapper {
public:
    TCapabilityWrapper(const TScenarioRequestWrapper& request, const TEnvironmentState* environmentState = nullptr)
        : Request_{request}
        , EnvironmentState_{environmentState}
    {
    }

    bool HasLedDisplay() const {
        return Request_.Proto().GetBaseRequest().GetInterfaces().GetHasLedDisplay();
    }

    bool HasScledDisplay() const {
        return Request_.Proto().GetBaseRequest().GetInterfaces().GetHasScledDisplay();
    }

    bool CanOpenLink() const {
        return Request_.Proto().GetBaseRequest().GetInterfaces().GetCanOpenLink();
    }

    bool CanRenderDivCards() const {
        return Request_.Proto().GetBaseRequest().GetInterfaces().GetCanRenderDivCards();
    }

    bool SupportsShowView() const {
        return Request_.Proto().GetBaseRequest().GetInterfaces().GetSupportsShowView();
    }

    bool SupportsCloudUi() const {
        return Request_.Proto().GetBaseRequest().GetInterfaces().GetSupportsCloudUi();
    }

    bool SupportsS3Animations() const {
        if (!EnvironmentState_) {
            return false;
        }
        return DeviceSupportsS3Animations(*EnvironmentState_, Request_.ClientInfo().DeviceId);
    }

    bool SupportsClientBiometry() const {
        if (!EnvironmentState_) {
            return false;
        }
        return DeviceSupportsBiometry(*EnvironmentState_, Request_.ClientInfo().DeviceId);
    }

private:
    const TScenarioRequestWrapper& Request_;
    const TEnvironmentState* EnvironmentState_;
};

// TODO(zhigan): refactor this
template<>
inline bool TCapabilityWrapper<TScenarioRunRequestWrapper>::SupportsS3Animations() const {
    const TEnvironmentState* environmentState = GetEnvironmentStateProto(Request_);
    if (!environmentState) {
        return false;
    }
    return DeviceSupportsS3Animations(*environmentState, Request_.ClientInfo().DeviceId);
}

template<>
inline bool TCapabilityWrapper<TScenarioRunRequestWrapper>::SupportsClientBiometry() const {
    return NAlice::NHollywood::SupportsClientBiometry(Request_);
}

} // NAlice::NHollywood
