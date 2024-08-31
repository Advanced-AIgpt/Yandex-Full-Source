#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>
#include <alice/hollywood/library/modifiers/registry/modifier_registry.h>

namespace NAlice::NHollywood::NModifiers {

namespace NImpl {

bool IsApplicable(IModifierContext& ctx, const TResponseBodyBuilder& responseBody);

TApplyResult TryApplyImpl(IModifierContext& ctx, TResponseBodyBuilder& responseBody,
                          TModifierAnalyticsInfoBuilder& analyticsInfo);

} // namespace NImpl

class TWhisperModifier : public TBaseModifier {
public:
    TWhisperModifier();

    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;
};

} // namespace NAlice::NHollywood::NModifiers
