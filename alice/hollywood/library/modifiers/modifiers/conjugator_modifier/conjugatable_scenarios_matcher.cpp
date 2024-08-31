#include "conjugatable_scenarios_matcher.h"

namespace NAlice::NHollywood::NModifiers {

TConjugatableScenariosMatcher::TConjugatableScenariosMatcher(const TConjugatableScenariosConfig& conjugatableScenariosConfig) {
    for (const auto& languageConfig : conjugatableScenariosConfig.GetLanguageConfigs()) {
        const auto language = languageConfig.GetLanguage();
        for (const auto& productScenarioName : languageConfig.GetProductScenarioNames()) {
            EnabledLanguageScenarioTuples_.emplace(std::make_tuple(language, productScenarioName));
        }
    }
}

bool TConjugatableScenariosMatcher::IsConjugatableLanguageScenario(const ELang language, const TString& productScenarioName, const TExpFlags& expFlags) const {
    if (EnabledLanguageScenarioTuples_.contains(std::make_tuple(language, productScenarioName))) {
        return true;
    }

    if (IsExpFlagTrue(expFlags, EXP_CONJUGATOR_MODIFIER_ENABLE_SCENARIO_PREFIX + productScenarioName)) {
        return true;
    }

    return false;
}

} // namespace NAlice::NHollywood::NModifiers
