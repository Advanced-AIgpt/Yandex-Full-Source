#pragma once
#include <alice/cuttlefish/library/experiments/experiment_patch.h>

namespace NVoice::NExperiments {

class TEventPatcher {
public:
    TEventPatcher(TVector<const TExpPatch*>&& commonPatches, TVector<TExpPatch>&& uniquePatches = {});

    void Patch(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& context) const;

private:
    TVector<TExpPatch> UniquePatches;
    TVector<const TExpPatch*> CommonPatches;
};

}  // namespace NVoice::NExperiments
