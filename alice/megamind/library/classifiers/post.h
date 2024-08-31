#pragma once

#include <alice/megamind/library/request/request.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <util/generic/fwd.h>

class TFactorStorage;

namespace NAlice {

class IContext;
class TFormulasStorage;
class TQualityStorage;
class TScenarioConfigRegistry;
class TScenarioResponse;

/**
 * PostClassify ranks scenarioResponses (front is the best)
 * Also fills some predicts in qualityStorage
 */
void PostClassify(
    const IContext& ctx,
    const NScenarios::TInterfaces& interfaces,
    const TMaybe<TRequest::TScenarioInfo>& boostedScenario,
    const TVector<TSemanticFrame>& recognizedActionEffectFrames,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TFormulasStorage& formulasStorage,
    const TFactorStorage& factorStorage,
    TVector<TScenarioResponse>& scenarioResponses,
    TQualityStorage& qualityStorage
);

} // namespace NAlice
