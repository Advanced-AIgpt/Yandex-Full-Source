#include "conjugatable_scenarios_matcher.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NHollywood;
using namespace NAlice;


TConjugatableScenariosConfig CreateTestConfig() {
    TConjugatableScenariosConfig result;
    {
        auto& languageConfig = *result.AddLanguageConfigs();
        languageConfig.SetLanguage(ELang::L_ARA);
        *languageConfig.AddProductScenarioNames() = "arabic_scenario";
        *languageConfig.AddProductScenarioNames() = "common_scenario";
    }
    {
        auto& languageConfig = *result.AddLanguageConfigs();
        languageConfig.SetLanguage(ELang::L_ENG);
        *languageConfig.AddProductScenarioNames() = "english_scenario";
        *languageConfig.AddProductScenarioNames() = "common_scenario";
    }
    return result;
}

Y_UNIT_TEST_SUITE(ConjugatableScenariosMatcher) {
    Y_UNIT_TEST(TestMatcher) {
        const auto config = CreateTestConfig();
        const auto matcher = TConjugatableScenariosMatcher(config);

        {
            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_RUS, "common_scenario", TExpFlags()));
            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_RUS, "arabic_scenario", TExpFlags()));
            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_RUS, "english_scenario", TExpFlags()));
        }

        {
            UNIT_ASSERT(matcher.IsConjugatableLanguageScenario(
                ELang::L_ARA, "common_scenario", TExpFlags()));
            UNIT_ASSERT(matcher.IsConjugatableLanguageScenario(
                ELang::L_ARA, "arabic_scenario", TExpFlags()));
            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_ARA, "english_scenario", TExpFlags()));
        }

        {
            UNIT_ASSERT(matcher.IsConjugatableLanguageScenario(
                ELang::L_ENG, "common_scenario", TExpFlags()));
            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_ENG, "arabic_scenario", TExpFlags()));
            UNIT_ASSERT(matcher.IsConjugatableLanguageScenario(
                ELang::L_ENG, "english_scenario", TExpFlags()));
        }

        {
            const auto expFlags = TExpFlags {
                {EXP_CONJUGATOR_MODIFIER_ENABLE_SCENARIO_PREFIX + TString("arabic_scenario"), "1"}
            };

            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_RUS, "common_scenario", expFlags));
            UNIT_ASSERT(matcher.IsConjugatableLanguageScenario(
                ELang::L_RUS, "arabic_scenario", expFlags));
            UNIT_ASSERT(!matcher.IsConjugatableLanguageScenario(
                ELang::L_RUS, "english_scenario", expFlags));
        }
    }
}

}
