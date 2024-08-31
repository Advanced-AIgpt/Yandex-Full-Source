#include "happy_new_year.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/happy_new_year/nlg/register.h>

namespace NAlice::NHollywood {

    REGISTER_SCENARIO(
        "happy_new_year",
        AddHandle<THappyNewYearRunHandle>()
            .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NHappyNewYear::NNlg::RegisterAll)
    );

} // namespace NAlice::NHollywood
