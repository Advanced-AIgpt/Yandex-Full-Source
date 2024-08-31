#pragma once

#include <util/generic/strbuf.h>

namespace NAlice {

    namespace NScenarios {
        class TDirective;
    } // namespace NScenarios

    class TEnvironmentState;

    enum TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy : int;
} // namespace NAlice

namespace NAlice::NHollywood {

bool DeviceSupportsS3Animations(const TEnvironmentState& environmentState, TStringBuf deviceId);

NScenarios::TDirective BuildDrawAnimationDirective(TStringBuf animationPath,
    TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy policy = static_cast<TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy>(0));

} // namespace NAlice::NHollywood
