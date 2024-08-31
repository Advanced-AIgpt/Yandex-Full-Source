#include "matrixnet.h"

#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>

#include <kernel/factor_storage/factor_storage.h>
#include <kernel/matrixnet/relev_calcer.h>

namespace NAlice {

[[nodiscard]] double ApplyScenarioFormula(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage stage,
    const EClientType clientType,
    const TStringBuf experiment,
    const TFactorStorage& storage,
    TRTLogger& logger,
    const ELanguage language
) {
    auto [calcer, description] = formulasStorage.Lookup(
        scenarioName,
        stage,
        clientType,
        experiment,
        language
    );
    if (!calcer) {
        return 0.0;
    }

    Y_ENSURE(description);
    LOG_DEBUG(logger) << " Use formula " << description->GetFormulaName() << " for scenario " << scenarioName;

    TVector<double> predicts(Reserve(1));
    calcer->SlicedCalcRelevs({&storage}, predicts);
    Y_ENSURE(predicts.size() == 1);

    return predicts.front();
}

} // namespace NAlice
