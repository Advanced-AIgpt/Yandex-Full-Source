#include <alice/hollywood/library/scenarios/time_capsule/handles/time_capsule_run.h>

#include <alice/hollywood/library/scenarios/time_capsule/nlg/register.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NTimeCapsule {

REGISTER_SCENARIO("time_capsule",
                  AddHandle<TTimeCapsuleRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTimeCapsule::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTimeCapsule
