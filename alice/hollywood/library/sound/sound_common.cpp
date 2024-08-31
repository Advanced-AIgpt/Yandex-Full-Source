#include "sound_common.h"

#include <alice/megamind/protos/common/device_state.pb.h>

#include <util/generic/fwd.h>


namespace NAlice::NHollywood {

void AddMultiroomSessionIdToDirectiveValue(NJson::TJsonValue& directiveValue, const TDeviceState& deviceState) {
    if (deviceState.HasMultiroom() && deviceState.GetMultiroom().HasMultiroomSessionId()) {
        directiveValue[MULTIROOM_SESSION_ID] = deviceState.GetMultiroom().GetMultiroomSessionId();
    }
}

TMaybe<TFrame> GetFrame(const TScenarioInputWrapper& input, const TVector<TStringBuf>& frames) {
    for (const auto& frame : frames) {
        if (input.FindSemanticFrame(frame)) {
            return input.CreateRequestFrame(frame);
        }
    }
    return Nothing();
}

} // namespace NAlice::NHollywood
