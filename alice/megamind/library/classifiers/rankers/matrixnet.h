#pragma once

#include <alice/megamind/library/classifiers/formulas/protos/formulas_description.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/fwd.h>

class TFactorStorage;

namespace NAlice {

class TFormulasStorage;

/**
 * Function applies matrixnet/catboost formula for scenario by its attributes
 * Returns prediction for further ranking
 */
[[nodiscard]] double ApplyScenarioFormula(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage stage,
    const EClientType clientType,
    const TStringBuf experiment,
    const TFactorStorage& storage,
    TRTLogger& logger,
    const ELanguage language
);

} // namespace NAlice
