#include "priority.h"

#include <alice/megamind/library/classifiers/defs/priorities.h>
#include <alice/megamind/library/classifiers/util/experiments.h>
#include <alice/megamind/library/classifiers/util/scenario_specific.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/scenarios/defs/names.h>

#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/video_common/defs.h>

namespace NAlice {

namespace {

using TPriorityMap = THashMap<TStringBuf, double>;

const TPriorityMap MM_SCENARIOS_PRIORITIES = {
    {MM_ALICE4BUSINESS_SCENARIO, 1.0},
    // Alice4Business goes even before IOT. Device is locked means it IS locked.
    {MM_IOT_PROTOCOL_SCENARIO, 1 - DBL_EPSILON},
    {MM_IOT_SCENARIOS_PROTOCOL_SCENARIO, 1 - DBL_EPSILON},
    // HardcodedResponse goes after IOT because, if user wants to turn on his vacuum cleaner with utterance
    // "Toss a coin to your Witcher", we should turn on vacuum cleaner instead of answering "Oh, valley of plenty"
    {HARDCODED_RESPONSE_SCENARIO, 1 - 2 * DBL_EPSILON},
    {MM_VIDEO_CALL_PROTOCOL_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY + DBL_EPSILON}, // boost VideoCall priority to win over MessengerCall
    {HOLLYWOOD_HARDCODED_MUSIC_SHOW_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - DBL_EPSILON}, // boost show priority to win over the old one
    {MM_MARKET_STATUS_ORDER, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - DBL_EPSILON}, // boost order priority to win over market_status_order (remove after DIALOG-8858)
    {MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO, 0.65}, // loses to other protocol scenarios but wins any gc
    {MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO, 0.65}, // loses to other protocol scenarios but wins any gc
    {MM_PROTO_VINS_SCENARIO, NMegamind::MM_POST_CLASSIFY_PRIORITY},
    {MM_VIDEO_PROTOCOL_SCENARIO, NMegamind::MM_POST_CLASSIFY_PRIORITY},
    {MM_SEARCH_PROTOCOL_SCENARIO, NMegamind::MM_POST_CLASSIFY_PRIORITY},
    {HOLLYWOOD_MUSIC_SCENARIO, NMegamind::MM_POST_CLASSIFY_PRIORITY},
    {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, NMegamind::MM_SWAP_TRICK_PRIORITY}, // lower MM_POST_CLASSIFY_PRIORITY, replace vins gc
    {MM_GENERAL_CONVERSATION_SCENARIO, NMegamind::MM_SWAP_TRICK_PRIORITY},
    {MM_MARKET_HOW_MUCH, NMegamind::MM_SWAP_TRICK_PRIORITY},
    // Any protocol scenario can catch some command from Commands scenario and should be prioritized
    {HOLLYWOOD_COMMANDS_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - DBL_EPSILON},
    {SIDE_SPEECH_SCENARIO, 0.1}, // Should only win if explicitly specified by SideSpeech classifier
};

const TPriorityMap MM_SCENARIOS_TURKISH_PRIORITIES = {
    {MM_NAVI_EXTERNAL_CONFIRMATION_TR_PROTOCOL_SCENARIO, 1.0},
    {MM_ADD_POINT_TR_PROTOCOL_SCENARIO, 0.99},
    {MM_GET_MY_LOCATION_TR_PROTOCOL_SCENARIO, 0.97},
    {MM_SWITCH_LAYER_TR_PROTOCOL_SCENARIO, 0.93},
    {MM_GET_WEATHER_TR_PROTOCOL_SCENARIO, 0.86},
    {MM_SHOW_ROUTE_TR_PROTOCOL_SCENARIO, 0.83},
    {MM_HANDCRAFTED_TR_PROTOCOL_SCENARIO, 0.75},
    {MM_FIND_POI_TR_PROTOCOL_SCENARIO, 0.65},
    {MM_GENERAL_CONVERSATION_TR_PROTOCOL_SCENARIO, 0.55},
};

const TPriorityMap MM_SCENARIOS_ARABIC_PRIORITIES = {
    // Alice4Business goes even before IOT. Device is locked means it IS locked.
    {MM_ALICE4BUSINESS_SCENARIO, 1.0},
    {MM_IOT_PROTOCOL_SCENARIO, 1 - DBL_EPSILON},
    // HardcodedResponse goes after IOT because, if user wants to turn on his vacuum cleaner with utterance
    // "Toss a coin to your Witcher", we should turn on vacuum cleaner instead of answering "Oh, valley of plenty"
    {HARDCODED_RESPONSE_SCENARIO, 1 - 2 * DBL_EPSILON},
    // {DEFAULT_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY} -- default scenario priority
    // Music is nearly protocol in arabic because it has frame classifiers in begemot
    // Music has lower priority than other scenarios to lose to more specific scenario
    {HOLLYWOOD_MUSIC_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - DBL_EPSILON},
    // Any protocol scenario can catch some command from Commands scenario and should be prioritized
    {HOLLYWOOD_COMMANDS_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - 2 * DBL_EPSILON},
    // here goes scenarios that must be postclassified
    // now we don't have classifiers for them, so rank them somehow
    // Search (facts) on arabic answers with irrelevant more often than others so give it more priority
    // Translated VINS can answer with irrelevant
    // Translated Video and native GC do not answer with irrelevant, so we need classifiers for them
    {MM_SEARCH_PROTOCOL_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - 3 * DBL_EPSILON},
    {MM_PROTO_VINS_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - 4 * DBL_EPSILON},
    {MM_VIDEO_PROTOCOL_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - 5 * DBL_EPSILON},
    {PROTOCOL_GENERAL_CONVERSATION_SCENARIO, NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY - 6 * DBL_EPSILON},
};

[[nodiscard]] const TPriorityMap* SelectPriorityMap(const ELanguage lang) {
    switch (lang) {
        case ELanguage::LANG_RUS:
            return &MM_SCENARIOS_PRIORITIES;
        case ELanguage::LANG_TUR:
            return &MM_SCENARIOS_TURKISH_PRIORITIES;
        case ELanguage::LANG_ARA:
            return &MM_SCENARIOS_ARABIC_PRIORITIES;
        default:
            return &MM_SCENARIOS_PRIORITIES;
    }
}
} // namespace

namespace NImpl {

[[nodiscard]] bool HasFormulasForClassification(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage classificationStage,
    const TClientFeatures& clientFeatures,
    const THashMap<TString, TMaybe<TString>>& experiments,
    const ELanguage language
) {
    const auto clientType = GetClientType(clientFeatures);
    const TMaybe<TStringBuf> experiment = NMegamind::GetMMFormulaExperimentForSpecificClient(experiments, clientType);
    return formulasStorage.Contains(
        scenarioName,
        classificationStage,
        clientType,
        experiment.GetOrElse({}),
        language
    );
}

} // namespace NImpl

[[nodiscard]] double GetScenarioPriority(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TStringBuf name,
    const TFormulasStorage& formulasStorage
) {
    if (ctx.LanguageForClassifiers() == LANG_RUS) {
        // hacks for russian VINS swap tricks

        const bool hasFormulas = NImpl::HasFormulasForClassification(formulasStorage, name, ECS_POST, ctx.ClientFeatures(), ctx.ExpFlags(), ctx.LanguageForClassifiers());

        if (name == HOLLYWOOD_MUSIC_SCENARIO && !hasFormulas) {
            return NMegamind::MM_SWAP_TRICK_PRIORITY;
        }

        if (name == MM_SEARCH_PROTOCOL_SCENARIO && !hasFormulas) {
            return NMegamind::MM_SWAP_TRICK_PRIORITY;
        }

        if (name == PROTOCOL_GENERAL_CONVERSATION_SCENARIO && hasFormulas) {
            return NMegamind::MM_POST_CLASSIFY_PRIORITY;
        }

        if (name == MM_MARKET_HOW_MUCH && ctx.HasExpFlag(EXP_HOW_MUCH_BIN_CLASS)) {
            return NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY;
        }
    }

    const auto* priorities = SelectPriorityMap(ctx.LanguageForClassifiers());

    if (priorities) {
        if (const double* priority = priorities->FindPtr(name)) {
            return *priority;
        }
    }

    const auto& scenarioConfigs = scenarioRegistry.GetScenarioConfigs();
    if (const auto it = scenarioConfigs.find(name); it != scenarioConfigs.end()) {
        return NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY;
    }

    return NMegamind::MM_DISABLED_SCENARIO_PRIORITY;
}

} // namespace NAlice
