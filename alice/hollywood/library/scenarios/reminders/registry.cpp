#include "entry_point.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/reminders/nlg/register.h>

namespace NAlice::NHollywood::NReminders {

REGISTER_SCENARIO("reminders",
                  AddHandle<TRemindersEntryPointHandler>()
                  .SetNlgRegistration(NLibrary::NScenarios::NReminders::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood::NReminders
