#pragma once

#include <util/generic/maybe.h>

namespace NAlice {
    namespace NScenarios {
        class TDirective;
    } // namespace NScenarios
    class TEnvironmentState;
    enum TEqualizerCapability_EPresetMode : int;
} // namespace NAlice

namespace NAlice {

TMaybe<NScenarios::TDirective> TryBuildEqualizerBandsDirective(
    const TEnvironmentState& environmentState,
    TStringBuf seed,
    TStringBuf deviceId,
    TEqualizerCapability_EPresetMode presetMode);

} // namespace NAlice
