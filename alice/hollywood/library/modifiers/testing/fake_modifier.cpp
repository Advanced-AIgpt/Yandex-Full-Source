#include "fake_modifier.h"

namespace NAlice::NHollywood::NModifiers {

TFakeModifier::TFakeModifier(const TString& modifierType, const TApplyResult applyResult, bool enabled)
    : TBaseModifier{modifierType}
    , ApplyResult{applyResult} {
    SetEnabled(enabled);
}

TApplyResult TFakeModifier::TryApply(TModifierApplyContext applyCtx) const {
    Y_UNUSED(applyCtx);
    return ApplyResult;
}

} // namespace NAlice::NHollywood::NModifiers
