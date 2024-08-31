#include "continue.h"

#include <alice/megamind/library/classifiers/features/calcers.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/classifiers/rankers/matrixnet.h>
#include <alice/megamind/library/classifiers/util/experiments.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <kernel/factor_storage/factor_storage.h>

using namespace NAlice::NMegamind;

namespace NAlice {
namespace NImpl {

double TFormulaApplier::Predict(const IContext& ctx, const TFactorStorage& factorStorage,
                                const TFormulasStorage& formulasStorage, TStringBuf scenarioName,
                                TStringBuf experiment, EClientType clientType,
                                const TScenarioResponse& response) const {
    // FIXME (a-sidorin@): It is better to fill the initial storage instead as it doesn't require storage copy
    // and allows early feature filling. This requires some storage redesign, unfortunately.
    TFactorStorage factors = factorStorage;
    const auto& features = response.GetFeatures().GetScenarioFeatures();
    FillScenarioFactors(features, factors);
    return ApplyScenarioFormula(formulasStorage, scenarioName, ECS_POST, clientType, experiment, factors,
                                ctx.Logger());
}

bool IsScenarioAllowedForEarlyContinue(const IContext& ctx, const TFactorStorage& factorStorage,
                                       const TFormulasStorage& formulasStorage, NImpl::IFormulaApplier& formulaApplier,
                                       TStringBuf scenarioName, const TScenarioResponse& response) {
    if (!ctx.HasExpFlag(EXP_ENABLE_EARLY_CONTINUE)) {
        return false;
    }

    if (scenarioName != HOLLYWOOD_MUSIC_SCENARIO) {
        return false;
    }

    const auto clientType = GetClientType(ctx.ClientFeatures());
    const TStringBuf experiment = GetMMFormulaExperimentForSpecificClient(ctx.ExpFlags(), clientType).GetOrElse({});

    auto [_, description] = formulasStorage.Lookup(scenarioName, ECS_POST, clientType, experiment);
    if (!description->HasContinueThreshold()) {
        return false;
    }
    const double threshold = description->GetContinueThreshold().value();
    const double predict =
        formulaApplier.Predict(ctx, factorStorage, formulasStorage, scenarioName, experiment, clientType, response);

    LOG_INFO(ctx.Logger()) << "Scenario " << scenarioName << " predict value " << predict << " vs threshold "
                           << threshold;
    return predict >= threshold;
}

} // namespace NImpl

bool IsScenarioAllowedForEarlyContinue(const IContext& ctx, const TFactorStorage& factorStorage,
                                       const TFormulasStorage& formulasStorage, TStringBuf scenarioName,
                                       const TScenarioResponse& response) {
    NImpl::TFormulaApplier formulaApplier;
    return NImpl::IsScenarioAllowedForEarlyContinue(ctx, factorStorage, formulasStorage, formulaApplier, scenarioName,
                                                    response);
}

} // namespace NAlice
