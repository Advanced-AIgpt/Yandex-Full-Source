#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>

namespace NAlice::NHollywood::NModifiers {

// can't do mock because of template methods
class TFakeModifier : public TBaseModifier {
public:
    TFakeModifier(const TString& modifierType,
                  const TApplyResult applyResult = TNonApply{TNonApply::EType::NotApplicable}, bool enabled = false);

    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;

private:
    const TApplyResult ApplyResult;
};

} // namespace NAlice::NHollywood::NModifiers
