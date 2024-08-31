#include "fresh_options.h"

#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/begemot/lib/rule_utils/rule_input.h>

#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>

namespace NAlice {

void ReadAliceFreshOptions(const NBg::TInput& input, TFreshOptions* options) {
    Y_ENSURE(options);
    if (HasWizextraFlag(input, EXP_BEGEMOT_FRESH_ALICE)) {
        options->SetForceEntireFresh(true);
    }
    for (const TString& name : GetWizextraParamAllValues(input, EXP_BEGEMOT_FRESH_ALICE_FORM)) {
        options->AddForceForForms(name);
    }
    for (const TString& name : GetWizextraParamAllValues(input, EXP_BEGEMOT_FRESH_ALICE_ENTITY)) {
        options->AddForceForEntities(name);
    }
    for (const TString& name : GetWizextraParamAllValues(input, EXP_BEGEMOT_FRESH_ALICE_PREFIX)) {
        options->AddForceForPrefixes(name);
    }
    // ForceForExperiments is deprecated. Only for granet backward compatibility.
}

void ReadGranetFreshOptions(const NBg::TInput& input, TFreshOptions* options) {
    Y_ENSURE(options);
    if (HasWizextraFlag(input, EXP_BEGEMOT_FRESH_GRANET)) {
        options->SetForceEntireFresh(true);
    }
    for (const TString& name : GetWizextraParamAllValues(input, EXP_BEGEMOT_FRESH_GRANET_FORM)) {
        options->AddForceForForms(name);
    }
    for (const TString& name : GetWizextraParamAllValues(input, EXP_BEGEMOT_FRESH_GRANET_ENTITY)) {
        options->AddForceForEntities(name);
    }
    for (const TString& name : GetWizextraParamAllValues(input, EXP_BEGEMOT_FRESH_GRANET_PREFIX)) {
        options->AddForceForPrefixes(name);
    }
    // Flag bg_fresh_granet_experiment is deprecated
    for (const TString& name : GetWizextraParamAllValues(input, "bg_fresh_granet_experiment")) {
        options->AddForceForExperiments(name);
    }
    // ForceForFeatures is not relevant for granet.
}

bool ShouldUseFreshForForm(const TFreshOptions& freshOptions, TStringBuf name) {
    return freshOptions.GetForceEntireFresh()
        || IsIn(freshOptions.GetForceForForms(), name)
        || AnyOf(freshOptions.GetForceForPrefixes(), [name](const TString& prefix) {return name.StartsWith(prefix);});
}

} // namespace NAlice
