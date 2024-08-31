#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>
#include <alice/hollywood/library/modifiers/registry/modifier_registry.h>

namespace NAlice::NHollywood::NModifiers {

class TCloudUiModifier : public TBaseModifier {
public:
    TCloudUiModifier();

    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;
};

} // namespace NAlice::NHollywood::NModifiers
