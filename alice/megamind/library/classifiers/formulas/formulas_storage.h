#pragma once

#include "formulas_description.h"

#include <alice/megamind/library/classifiers/formulas/protos/formulas_description.pb.h>

#include <kernel/formula_storage/shared_formulas_adapter/shared_formulas_adapter.h>
#include <kernel/matrixnet/relev_calcer.h>
#include <library/cpp/langs/langs.h>

namespace NAlice {

class TFormulasStorage {
public:
    TFormulasStorage(const ISharedFormulasAdapter& storage, const TFormulasDescription& description)
        : Storage_{storage}
        , Description_{description}
    {
    }

    std::pair<TAtomicSharedPtr<NMatrixnet::IRelevCalcer>, const TFormulaDescription*> Lookup(
        const TStringBuf scenarioName,
        const EMmClassificationStage classificationStage,
        const EClientType clientType,
        const TStringBuf experiment,
        const ELanguage language
    ) const noexcept;

    bool Contains(
        const TStringBuf scenarioName,
        const EMmClassificationStage classificationStage,
        const EClientType clientType,
        const TStringBuf experiment,
        const ELanguage language
    ) const noexcept;

private:
    const ISharedFormulasAdapter& Storage_;
    const TFormulasDescription& Description_;
};

} // namespace NAlice
