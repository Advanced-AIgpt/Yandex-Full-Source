#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>
#include <alice/hollywood/library/modifiers/matchers/exact_matcher.h>
#include <alice/hollywood/library/modifiers/registry/modifier_registry.h>

namespace NAlice::NHollywood::NModifiers {

namespace NImpl {

TApplyResult TryApplyImpl(IModifierContext& ctx, TResponseBodyBuilder& responseBody,
                          TModifierAnalyticsInfoBuilder& analyticsInfo, const TExactMatcher& matcher);

} // namespace NImpl

class TColoredSpeakerModifier : public TBaseModifier {
public:
    TColoredSpeakerModifier();

    void LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) override;

    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;
private:
    std::unique_ptr<TExactMatcher> Matcher_;
};

} // namespace NAlice::NHollywood::NModifiers
