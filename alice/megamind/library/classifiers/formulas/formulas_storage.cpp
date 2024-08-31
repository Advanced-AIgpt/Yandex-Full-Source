#include "formulas_storage.h"

namespace NAlice {

namespace {

// When we add new scenario classifier to default config, we could disable it for experiments
// If we delete scenario, formula with old_ranking flag would be found
constexpr TStringBuf DISABLE_DEFAULT_FLAG = "old_ranking";

} // namespace

std::pair<TAtomicSharedPtr<NMatrixnet::IRelevCalcer>, const TFormulaDescription*> TFormulasStorage::Lookup(
    const TStringBuf scenarioName,
    const EMmClassificationStage classificationStage,
    const EClientType clientType,
    const TStringBuf experiment,
    const ELanguage language
) const noexcept {
    const auto* formula = &Description_.Lookup(
        scenarioName,
        classificationStage,
        clientType,
        experiment,
        language
    );
    auto calcer = Storage_.GetSharedFormula(formula->GetFormulaName());
    if (!calcer && experiment != DISABLE_DEFAULT_FLAG) {
        formula = &Description_.Lookup(
            scenarioName,
            classificationStage,
            clientType,
            {},
            language
        );
        calcer = Storage_.GetSharedFormula(formula->GetFormulaName());
    }
    return std::make_pair(calcer, formula);
}

bool TFormulasStorage::Contains(
    const TStringBuf scenarioName,
    const EMmClassificationStage classificationStage,
    const EClientType clientType,
    const TStringBuf experiment,
    const ELanguage language
) const noexcept {
    const auto [calcer, _] = Lookup(
        scenarioName,
        classificationStage,
        clientType,
        experiment,
        language
    );
    return calcer.Get();
}

} // namespace NAlice
