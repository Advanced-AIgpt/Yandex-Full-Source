#pragma once

#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/library/client/client_info.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/fwd.h>

namespace NAlice {

class IContext;
class TScenarioConfigRegistry;

namespace NImpl {

bool HasFormulasForClassification(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage classificationStage,
    const TClientFeatures& clientFeatures,
    const THashMap<TString, TMaybe<TString>>& experiments,
    const ELanguage language
);

} // namespace NImpl

/**
 * Function looks for human selected priority for scenario by name
 * Returns priority (or -1 if it was not found) for further ranking
 */
[[nodiscard]] double GetScenarioPriority(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TStringBuf name,
    const TFormulasStorage& formulasStorage
);

} // namespace NAlice
