#pragma once

#include <alice/hollywood/library/modifiers/modifiers/conjugator_modifier/conjugatable_scenarios_matcher.h>
#include <alice/hollywood/library/modifiers/base_modifier/base_modifier.h>


namespace NAlice::NHollywood::NModifiers {

constexpr TStringBuf AR_CONJUGATOR_REQUEST_ITEM_NAME = "ar_conjugator_request";
constexpr TStringBuf CONJUGATOR_REQUEST_ITEM_NAME = "conjugator_request";
constexpr TStringBuf CONJUGATOR_RESPONSE_ITEM_NAME = "conjugator_response";

class TConjugatorModifier : public TBaseModifier {
public:
    TConjugatorModifier();

    void LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) override;

    void Prepare(TModifierPrepareContext prepareCtx) const override;

    TApplyResult TryApply(TModifierApplyContext applyCtx) const override;
public:
    void Configure(const TConjugatableScenariosConfig& conjugatableScenariosConfig);
private:
    std::unique_ptr<TConjugatableScenariosMatcher> ConjugatableScenariosMatcher_;
};

} // namespace NAlice::NHollywood::NModifiers
