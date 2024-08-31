#include "base_modifier.h"

#include <util/string/cast.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

inline constexpr TStringBuf EXP_DISABLE_MODIFIER_PREFIX = "mm_disable_modifier=";

} // namespace

TNonApply::TNonApply(EType type)
    : Type_(type)
{
}

TNonApply::TNonApply(const TString& reason)
    : Type_(EType::ModSpecific)
    , Reason_(reason)
{
}

TNonApply::EType TNonApply::Type() const {
    return Type_;
}

TString TNonApply::Reason() const {
    if (Reason_) {
        return *Reason_;
    }
    return ToString(Type_);
}

void TBaseModifier::SetEnabled(bool enabled) {
    Enabled_ = enabled;
}

bool TBaseModifier::IsEnabled(const IModifierContext& ctx) const {
    return !ctx.HasExpFlag(EXP_DISABLE_MODIFIER_PREFIX + GetModifierType()) &&
           (Enabled_ || ctx.HasExpFlag(EXP_ENABLE_MODIFIER_PREFIX + GetModifierType()));
}

} // namespace NAlice::NHollywood::NModifiers
