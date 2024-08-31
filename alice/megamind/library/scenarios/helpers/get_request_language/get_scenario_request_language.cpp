#include "get_scenario_request_language.h"
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>
#include <alice/megamind/library/experiments/flags.h>


namespace NAlice::NMegamind {

ELanguage GetScenarioRequestLanguage(const TScenarioConfig& config, const IContext& ctx) {
    const auto language = ctx.Language();
    const auto ordinarLanguage = ConvertAliceWorldWideLanguageToOrdinar(language);
    if (language == ordinarLanguage) {
        return language;
    }

    if (ctx.HasExpFlag(EXP_FORCE_POLYGLOT_LANGUAGE_FOR_SCENARIO_PREFIX + config.GetName())) {
        return language;
    }
    if (ctx.HasExpFlag(EXP_FORCE_TRANSLATED_LANGUAGE_FOR_SCENARIO_PREFIX + config.GetName())) {
        return ordinarLanguage;
    }
    if (ctx.HasExpFlag(EXP_FORCE_TRANSLATED_LANGUAGE_FOR_ALL_SCENARIOS)) {
        return ordinarLanguage;
    }

    // Scenario that supports neither language nor ordinarLanguage should be filtered out by preclassify
    Y_ASSERT(IsIn(config.GetLanguages(), language) || IsIn(config.GetLanguages(), ordinarLanguage));

    return IsIn(config.GetLanguages(), language) ? language : ordinarLanguage;
}

}
