#include "event_patcher.h"
#include "utils.h"

namespace NVoice::NExperiments {

TEventPatcher::TEventPatcher(TVector<const TExpPatch*>&& commonPatches, TVector<TExpPatch>&& uniquePatches)
    : UniquePatches(std::move(uniquePatches))
    , CommonPatches(std::move(commonPatches))
{ }

void TEventPatcher::Patch(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& context) const
{
    EnsureVinsExperimentsFormat(&(event["payload"]["request"]["experiments"]));

    for (const TExpPatch& p : UniquePatches)
        p.Apply(event, context);
    for (const TExpPatch* p : CommonPatches)
        p->Apply(event, context);
}

}  // namespace NVoice::NExperiments
