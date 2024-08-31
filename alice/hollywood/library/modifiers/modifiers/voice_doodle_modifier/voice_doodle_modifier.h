#pragma once

#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>
#include <alice/hollywood/library/modifiers/matchers/exact_matcher.h>
#include <alice/hollywood/library/modifiers/registry/modifier_registry.h>

namespace NAlice::NHollywood::NModifiers {

class TVoiceDoodleModifier : public TBaseModifier {
public:
    TVoiceDoodleModifier();

    void LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) override;

    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;

private:
    std::unique_ptr<TExactMatcher> Matcher_;
};

} // namespace NAlice::NHollywood::NModifiers
