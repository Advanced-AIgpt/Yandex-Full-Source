#include "post.h"

#include <alice/megamind/library/classifiers/defs/priorities.h>
#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/classifiers/intents_by_priority/classifier.h>
#include <alice/megamind/library/classifiers/rankers/matrixnet.h>
#include <alice/megamind/library/classifiers/rankers/priority.h>
#include <alice/megamind/library/classifiers/util/experiments.h>
#include <alice/megamind/library/classifiers/util/force_response.h>
#include <alice/megamind/library/classifiers/util/modes.h>
#include <alice/megamind/library/classifiers/util/scenario_info.h>
#include <alice/megamind/library/classifiers/util/scenario_specific.h>
#include <alice/megamind/library/classifiers/util/thresholds.h>

#include <alice/megamind/protos/scenarios/features/vins.pb.h>
#include <alice/megamind/protos/scenarios/features/music.pb.h>

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/scenarios/helpers/get_request_language/get_scenario_request_language.h>
#include <alice/megamind/library/util/slot.h>

#include <alice/library/client/interfaces_util.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/metrics/names.h>
#include <alice/library/music/defs.h>
#include <alice/library/video_common/defs.h>

#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/iterator/mapped.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/string/join.h>

namespace NAlice {

namespace {

// if VINS chooses one of these intents, then its priority increases and trained postclassifier is ignored
const THashMap<ELanguage, THashSet<TStringBuf>> VINS_IMPORTANT_SCENARIOS = {
    {ELanguage::LANG_RUS, {
        TStringBuf("personal_assistant.scenarios.alarm_ask_sound"),
        TStringBuf("personal_assistant.scenarios.direct_gallery"),
        TStringBuf("personal_assistant.scenarios.ether.quasar.video_select"),
        TStringBuf("personal_assistant.scenarios.ether_show"),
        TStringBuf("personal_assistant.scenarios.repeat_after_me"),
        TStringBuf("personal_assistant.scenarios.quasar.go_up"),
        TStringBuf("personal_assistant.scenarios.quasar.go_down"),
        TStringBuf("personal_assistant.scenarios.quasar.go_top"),
        TStringBuf("personal_assistant.scenarios.image_gallery"),
        TStringBuf("personal_assistant.scenarios.what_is_my_name"),
        TStringBuf("personal_assistant.scenarios.set_my_name"),
        TStringBuf("personal_assistant.scenarios.alarm_snooze_rel"),
        TStringBuf("personal_assistant.scenarios.timer_show"),
    }},
};

const THashMap<ELanguage, TVector<TStringBuf>> VINS_IMPORTANT_SCENARIOS_PREFIXES = {
    {ELanguage::LANG_RUS, {
        TStringBuf("personal_assistant.scenarios.market"),
        TStringBuf("personal_assistant.scenarios.recurring_purchase"),
        TStringBuf("personal_assistant.scenarios.voiceprint"),
        TStringBuf("personal_assistant.internal.bugreport"),
        TStringBuf("personal_assistant.handcrafted.quasar"),
        TStringBuf("personal_assistant.scenarios.shopping_list"),
        TStringBuf("personal_assistant.scenarios.search_filter_"),
        TStringBuf("personal_assistant.navi.change_voice"),
        TStringBuf("personal_assistant.navi.hide_layer"),
        TStringBuf("personal_assistant.navi.add_point"),
        TStringBuf("personal_assistant.navi.add_point__ellipsis"),
        TStringBuf("personal_assistant.navi.add_point__cancel"),
        TStringBuf("personal_assistant.navi.how_long_to_drive"),
        TStringBuf("personal_assistant.navi.show_route_on_map"),
        TStringBuf("personal_assistant.navi.parking_route"),
        TStringBuf("personal_assistant.navi.reset_route"),
        TStringBuf("personal_assistant.navi.when_we_get_there"),
        TStringBuf("personal_assistant.navi.onboarding"),
    }},
};

// TODO(tolyandex) Try to remove handcrafted from important intent DIALOG-8823
const THashMap<ELanguage, THashSet<TStringBuf>> VINS_IMPORTANT_SCENARIOS_EXCEPTIONS = {
    {ELanguage::LANG_RUS, {
        TStringBuf("personal_assistant.handcrafted.quasar.future_skill_show_lyrics"),
    }},
};


struct TVinsImportantIntentCondition {
    TStringBuf IntentPrefix;
    TVector<TStringBuf> Grammars;
    std::function<bool(const IContext&)> CheckEnabled;
};

const THashMap<ELanguage, TVector<TVinsImportantIntentCondition>> VINS_IMPORTANT_SCENARIOS_PREFIXES_AND_GRAMMARS = {
    {ELanguage::LANG_RUS, {
        {
            TStringBuf("personal_assistant.scenarios.alarm"),
            {
                TStringBuf("personal_assistant.scenarios.alarm_general"),
            },
            [](const IContext&) { return true; },
        },
        {
            TStringBuf("personal_assistant.scenarios.music"),
            {
                TStringBuf("personal_assistant.scenarios.music_fairy_tale"),
            },
            [](const IContext& ctx) {
                const auto& wizardResponse = ctx.Responses().WizardResponse();
                const auto* generativeTaleActivationFrame = wizardResponse.GetRequestFrame("alice.generative_tale.activate");
                const auto* generativeTaleSharingFrame = wizardResponse.GetRequestFrame("alice.generative_tale.send_me_my_tale");
                const bool generativeTaleRequest = generativeTaleActivationFrame || generativeTaleSharingFrame;

                const bool disabledGlobally = ctx.HasExpFlag("mm_disable_fairy_tale_preferred_vins_intent");
                const bool disabledOnSmartSpeakers = ctx.ClientInfo().IsSmartSpeaker() &&
                    ctx.HasExpFlag("mm_disable_fairy_tale_preferred_vins_intent_on_smart_speakers");
                return !(disabledGlobally || disabledOnSmartSpeakers || generativeTaleRequest);
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.music"),
            {
                TStringBuf("personal_assistant.scenarios.music_sing_song"),
            },
            [](const IContext& ctx) { return !ctx.HasExpFlag("mm_disable_sing_song_preferred_vins_intent"); }
        },
        {
            TStringBuf("personal_assistant.scenarios.music"),
            {
                TStringBuf("personal_assistant.scenarios.music_ambient_sound"),
            },
            [](const IContext& ctx) { return !ctx.HasExpFlag("mm_disable_ambient_sound_preferred_vins_intent"); }
        },
        {
            TStringBuf("personal_assistant.scenarios.music_what_is_playing"),
            {
                TStringBuf("personal_assistant.scenarios.player.what_is_playing"),
            },
            [](const IContext& ctx) { return !ctx.HasExpFlag("mm_disable_what_is_playing_preferred_vins_intent"); }
        },
        {
            TStringBuf("personal_assistant.scenarios.tv_stream"),
            {
                TStringBuf("personal_assistant.scenarios.tv_stream"),
            },
            [](const IContext& ctx) { return !ctx.HasExpFlag("mm_disable_tv_stream_preferred_vins_intent"); }
        },
        {
            TStringBuf("personal_assistant.scenarios.create_reminder"),
            {
                TStringBuf("personal_assistant.scenarios.create_reminder"),
            },
            [](const IContext&) { return true; }
        },
        {
            TStringBuf("personal_assistant.scenarios.onboarding"),
            {
                TStringBuf("alice.onboarding.what_you_can_do"),
            },
            [](const IContext& ctx) { return !ctx.HasExpFlag("mm_disable_onboarding_preferred_vins_intent"); }
        },
    }},
};

const THashMap<ELanguage, TVector<std::pair<TStringBuf, TVector<TStringBuf>>>> VIDEO_IMPORTANT_SCENARIOS_PREFIXES_AND_GRAMMARS = {
    {ELanguage::LANG_RUS, {
        {
            TStringBuf("mm.personal_assistant.scenarios.quasar.open_current_video"),
            {
                TStringBuf("personal_assistant.scenarios.quasar.open_current_video"),
            }
        },
        {
            TStringBuf("mm.personal_assistant.scenarios.quasar.select_video_from_gallery"),
            {
                TStringBuf("personal_assistant.scenarios.select_video_by_number"),
                TStringBuf("personal_assistant.scenarios.quasar.select_video_from_gallery_by_text"),
            }
        },
        {
            TStringBuf("mm.personal_assistant.scenarios.quasar.payment_confirmed"),
            {
                TStringBuf("personal_assistant.scenarios.quasar.payment_confirmed"),
            }
        },
    }},
};

// VINS has a higher priority of these intents than the standard protocol priority.
// Priority of these intents is '0.7 + EPS'
const THashMap<ELanguage, THashSet<TStringBuf>> PREFERABLE_INTENTS_IN_VINS = {
    {ELanguage::LANG_RUS, {
        TStringBuf("personal_assistant.navi.show_layer"),
        TStringBuf("personal_assistant.navi.how_long_traffic_jam"),
    }},
};

struct TReplacementScenario {
    TStringBuf ScenarioName;
    std::function<bool(const IContext&)> CheckEnabled;
    TStringBuf ScenarioFrameName;
};

const THashMap<ELanguage, THashMap<TStringBuf, TReplacementScenario>> PREFERABLE_SCENARIO_TO_VINS_INTENT_AFTER_FORMULA = {
    {ELanguage::LANG_RUS, {
        {
            TStringBuf("personal_assistant.general_conversation.general_conversation"),
            {
                PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
                [](const IContext&) { return true; },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.pure_general_conversation"),
            {
                PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
                [](const IContext& ctx) { return !ctx.HasExpFlag(EXP_DISABLE_GC_PURE_PROTOCOL); },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.general_conversation.general_conversation_dummy"),
            {
                PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
                [](const IContext& ctx) { return !ctx.HasExpFlag(EXP_DISABLE_GC_FIXLIST_PROTOCOL); },
                {},
            }
        },
        {
            MM_MUSIC_SCENARIO_VINS,
            {
                HOLLYWOOD_MUSIC_SCENARIO,
                [](const IContext& ctx) {
                    return
                        !NMegamind::HasClassificationMusicFormulas(ctx.ClientInfo()) ||
                        (!ctx.HasExpFlag(EXP_MUSIC_PLAY_DISABLE_UNCONDITIONAL_SWAP_TRICK) && !ctx.ClientInfo().IsSmartSpeaker());
                },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.music_podcast"),
            {
                HOLLYWOOD_MUSIC_SCENARIO,
                [](const IContext&) { return true; },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.music_fairy_tale"),
            {
                HOLLYWOOD_MUSIC_SCENARIO,
                [](const IContext& ctx) {
                    const auto* fairyTaleOndemandFrame = ctx.Responses().WizardResponse().GetRequestFrame("alice.fairy_tale.ondemand");
                    return fairyTaleOndemandFrame && (
                        ctx.HasExpFlag(EXP_MUSIC_FAIRY_TALE_PREFER_HW_MUSIC_OVER_VINS) ||
                        ctx.ClientInfo().IsSmartSpeaker() && ctx.HasExpFlag(EXP_MUSIC_FAIRY_TALE_PREFER_HW_MUSIC_OVER_VINS_ON_SMART_SPEAKERS)
                    );
                },
                {}
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.radio_play"),
            {
                HOLLYWOOD_MUSIC_SCENARIO,
                [](const IContext& ctx) {
                    const auto* hwMusicRadioPlay = ctx.Responses().WizardResponse().GetRequestFrame("alice.music.fm_radio_play");
                    return hwMusicRadioPlay &&
                        ctx.ClientInfo().IsSmartSpeaker() && ctx.HasExpFlag(EXP_FM_RADIO_PREFER_HW_MUSIC_OVER_VINS_ON_SMART_SPEAKERS);
                },
                {}
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.how_much"),
            {
                MM_MARKET_HOW_MUCH,
                [](const IContext& ctx) {
                    return !ctx.HasExpFlag(EXP_HOW_MUCH_BIN_CLASS);
                },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.search"),
            {
                MM_SEARCH_PROTOCOL_SCENARIO,
                [](const IContext& ctx) {
                    return
                        !ctx.HasExpFlag(EXP_DISABLE_SEARCH_ACTIVATE_BY_VINS) &&
                        !ctx.ClientInfo().IsSmartSpeaker();
                },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.search_anaphoric"),
            {
                MM_SEARCH_PROTOCOL_SCENARIO,
                [](const IContext& ctx) {
                    return !ctx.HasExpFlag(EXP_DISABLE_SEARCH_ACTIVATE_BY_VINS);
                },
                {},
            }
        },
    }},
    {LANG_ARA, {
        // TODO(vl-trifonov, https://st.yandex-team.ru/DIALOG-8613) remove when GC is classified by begemot pre classifier
        {
            TStringBuf("personal_assistant.general_conversation.general_conversation"),
            {
                PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
                [](const IContext&) { return true; },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.scenarios.pure_general_conversation"),
            {
                PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
                [](const IContext& ctx) { return !ctx.HasExpFlag(EXP_DISABLE_GC_PURE_PROTOCOL); },
                {},
            }
        },
        {
            TStringBuf("personal_assistant.general_conversation.general_conversation_dummy"),
            {
                PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
                [](const IContext& ctx) { return !ctx.HasExpFlag(EXP_DISABLE_GC_FIXLIST_PROTOCOL); },
                {},
            }
        },
    }},
};

// Keep in sync with PREFERABLE_SCENARIO_TO_VINS_INTENT_AFTER_FORMULA
const THashMap<ELanguage, TVector<TStringBuf>> VINS_DEPENDENT_SCENARIOS = {
    {ELanguage::LANG_RUS, {
        MM_PROTO_VINS_SCENARIO,
        MM_MARKET_HOW_MUCH,
        PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
    }},
};

struct TBoostingCondition {
    TStringBuf IntentPrefix;
    TStringBuf Frame;
    std::function<bool(const IContext&)> CheckEnabled;
};

const THashMap<ELanguage, THashMultiMap<TStringBuf, TBoostingCondition>> SCENARIO_IMPORTANT_FRAMES = {
    {ELanguage::LANG_RUS, {
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("personal_assistant.scenarios.music_play_less"),
                TStringBuf("personal_assistant.scenarios.music_play_less"),
                [](const IContext& ctx) {
                    return !ctx.HasExpFlag(NExperiments::EXP_HOLLYWOOD_NO_MUSIC_PLAY_LESS);
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("personal_assistant.scenarios.music_play_anaphora"),
                TStringBuf("personal_assistant.scenarios.music_play_anaphora"),
                [](const IContext& ctx) {
                    return ctx.HasExpFlag(NExperiments::EXP_HOLLYWOOD_MUSIC_PLAY_ANAPHORA);
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("personal_assistant.scenarios.music_play"),
                TStringBuf("personal_assistant.scenarios.music_play"),
                [](const IContext& ctx) {
                    const auto* frame = ctx.Responses().WizardResponse().GetRequestFrame("personal_assistant.scenarios.music_play");
                    return frame && GetSlot(*frame, "stream");
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music_onboarding"),
                TStringBuf("alice.music_onboarding"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music.send_song_text"),
                TStringBuf("alice.music.send_song_text"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music.what_year_is_this_song"),
                TStringBuf("alice.music.what_year_is_this_song"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music.what_album_is_this_song_from"),
                TStringBuf("alice.music.what_album_is_this_song_from"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music.what_is_this_song_about"),
                TStringBuf("alice.music.what_is_this_song_about"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music_onboarding"),
                TStringBuf("alice.music_onboarding"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music_onboarding.tracks"),
                TStringBuf("alice.music_onboarding.tracks"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music_onboarding.artists"),
                TStringBuf("alice.music_onboarding.artists"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music_onboarding.genres"),
                TStringBuf("alice.music_onboarding.genres"),
                [](const IContext&) {
                    return true;
                }
            }
        },
        {
            HOLLYWOOD_MUSIC_SCENARIO,
            {
                TStringBuf("alice.music_onboarding.dont_know"),
                TStringBuf("alice.music_onboarding.dont_know"),
                [](const IContext&) {
                    return true;
                }
            }
        },
    }},
};

const THashMap<ELanguage, TVector<std::pair<TStringBuf, TVector<TStringBuf>>>> ERASE_DIRECT_CONDITIONS = {
    {ELanguage::LANG_RUS, {
        {
            TStringBuf("mm_erase_direct_on_open_site_or_app"),
            {
                TStringBuf("personal_assistant.scenarios.open_site_or_app"),
            }
        },
        {
            TStringBuf("mm_erase_direct_on_find_poi"),
            {
                TStringBuf("personal_assistant.scenarios.find_poi"),
            }
        },
    }},
};

TStringBuf GetVinsScenarioName(const IContext& ctx) {
    return ctx.IsProtoVinsEnabled() ? MM_PROTO_VINS_SCENARIO : MM_VINS_SCENARIO;
}

void FillScenariosWinLossReasons(const TVector<TScenarioResponse>& scenarioResponses, const IContext& ctx,
                                 const TScenarioConfigRegistry& scenarioRegistry, const TFormulasStorage& formulasStorage,
                                 TQualityStorage& qualityStorage)
{
    if (scenarioResponses.empty()) {
        return;
    }
    const auto priorityGetter = [&ctx, &scenarioRegistry, &formulasStorage](const TScenarioResponse& response) {
        return GetScenarioPriority(ctx, scenarioRegistry, response.GetScenarioName(), formulasStorage);
    };
    const auto winnerPriority = priorityGetter(scenarioResponses.front());
    const bool isWinnerPriorityPostClassify = winnerPriority == NMegamind::MM_POST_CLASSIFY_PRIORITY;
    if (qualityStorage.GetPostclassificationWinReason() == WR_UNKNOWN) {
        if (scenarioResponses.size() > 1 && winnerPriority == priorityGetter(scenarioResponses[1])) {
            qualityStorage.SetPostclassificationWinReason(isWinnerPriorityPostClassify ? WR_FORMULA : WR_RANDOM);
        } else {
            qualityStorage.SetPostclassificationWinReason(WR_PRIORITY);
        }
    }
    TVector<TStringBuf> probableWinnerScenarios = {scenarioResponses.front().GetScenarioName()};
    for (size_t i = 1; i < scenarioResponses.size(); ++i) {
        const auto& name = scenarioResponses[i].GetScenarioName();
        const auto priority = priorityGetter(scenarioResponses[i]);
        if (priority < winnerPriority) {
            UpdateScenarioClassificationInfo(qualityStorage, LR_PRIORITY, name, ECS_POST);
        } else {
            if (isWinnerPriorityPostClassify) {
                UpdateScenarioClassificationInfo(qualityStorage, LR_FORMULA, name, ECS_POST);
            } else {
                UpdateScenarioClassificationInfo(qualityStorage, LR_RANDOM, name, ECS_POST);
                probableWinnerScenarios.push_back(name);
            }
        }
    }
    if (probableWinnerScenarios.size() > 1) {
        LOG_WARN(ctx.Logger()) << "Ambiguous post classification: " <<
            JoinSeq(", ", probableWinnerScenarios);
        ctx.Sensors().IncRate(NSignal::POST_CLASSIFY_AMBIGUOUS_WIN);
    }
}

template <class TPredicate>
bool TryEraseIf(TVector<TScenarioResponse>& responses, TPredicate predicate, TQualityStorage& qualityStorage, const ELossReason lossReason) {
    bool hasErased = false;
    EraseIf(responses, [&predicate, &hasErased, &qualityStorage, &lossReason](const TScenarioResponse& response) {
        if (predicate(response)) {
            UpdateScenarioClassificationInfo(qualityStorage, lossReason, response.GetScenarioName(), ECS_POST);
            hasErased = true;
            return true;
        }
        return false;
    });
    return hasErased;
}

bool LeaveOnly(TStringBuf name, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage, const ELossReason lossReason) {
    return TryEraseIf(
        responses,
        [&name](const TScenarioResponse& response) { return response.GetScenarioName() != name; },
        qualityStorage,
        lossReason);
}

bool LeaveOnlyWithPriority(
    const TStringBuf name,
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const ELossReason lossReason,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    return TryEraseIf(
        responses,
        [&name, &ctx, &scenarioRegistry, &formulasStorage](const auto& response) {
            return response.GetScenarioName() != name
                && GetScenarioPriority(ctx, scenarioRegistry, response.GetScenarioName(), formulasStorage)
                   == GetScenarioPriority(ctx, scenarioRegistry, name, formulasStorage);
        },
        qualityStorage,
        lossReason
    );
}

bool LeaveOnlyWithPriorityAbovePostclassify(
        const TStringBuf name,
        const IContext& ctx,
        const TScenarioConfigRegistry& scenarioRegistry,
        const ELossReason lossReason,
        const TFormulasStorage& formulasStorage,
        TVector<TScenarioResponse>& responses,
        TQualityStorage& qualityStorage
) {
    return TryEraseIf(
        responses,
        [&name, &ctx, &scenarioRegistry, &formulasStorage](const auto& response) {
            return response.GetScenarioName() != name
                   && GetScenarioPriority(ctx, scenarioRegistry, response.GetScenarioName(), formulasStorage)
                      <= NMegamind::MM_POST_CLASSIFY_PRIORITY;
        },
        qualityStorage,
        lossReason
    );
}

const TScenarioResponse* GetScenarioResponse(const TVector<TScenarioResponse>& responses, const TStringBuf scenario) {
    return FindIfPtr(
        responses,
        [&scenario](const auto& response) { return response.GetScenarioName() == scenario; }
    );
}

constexpr TStringBuf VINS_GC_INTENT = "personal_assistant.general_conversation.general_conversation";

bool LeaveSkillDiscoveryOnlyOnGcIntent(TVector<TScenarioResponse>& responses, const TStringBuf vinsScenarioName, TQualityStorage& qualityStorage) {
    // Protocol GC is only activated when VINS response with GC. They are swapped at the end of post-classification process
    // check isPureGC in vins features even for protocol GC implementation
    if (const TScenarioResponse* vinsResponse = GetScenarioResponse(responses, vinsScenarioName)) {
        const auto& intent = vinsResponse->GetIntentFromFeatures();
        const bool isPureGC = vinsResponse->GetFeatures().GetScenarioFeatures().GetVinsFeatures().GetIsPureGC();
        if (intent == VINS_GC_INTENT && !isPureGC) {
            return false;
        }
    }

    return TryEraseIf(responses, [](const auto& response) {
        return response.GetScenarioName() == MM_DIALOGOVO_SKILLS_DISCOVERY_SCENARIO ||
               response.GetScenarioName() == MM_DIALOGOVO_SKILL_DISCOVERY_GC_SCENARIO ||
               response.GetScenarioName() == MM_DIALOGOVO_IMPLICIT_SKILL_DISCOVERY_SCENARIO;
    }, qualityStorage, LR_NOT_GC);
}

bool FilterScenariosByFixlist(const IContext& ctx, const NScenarios::TInterfaces& interfaces, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage) {
    /*
    If fixlist has matches "scenario", these scenarios are allowed.
    If fixlist has matches "scenario:feature", these scenarios are allowed, if this feature is supported.
    If there are scenarios allowed by fixlist, all the other scenarios are filtered out.
    */
    if (ctx.HasExpFlag(EXP_DISABLE_MM_INTENT_FIXLIST)) {
        return false;
    }

    const auto& fl = ctx.Responses().WizardResponse().GetFixlist();
    const THashSet<TString>& matches = fl.GetMatches();
    const THashMap<TString, THashSet<TString>>& featuresMatches = fl.GetSupportedFeaturesMatches();
    if (!matches && !featuresMatches) {
        return false;
    }
    THashSet<TStringBuf> allowedScenarios;
    for (const TScenarioResponse& scenarioResponse : responses) {
        const auto& scenarioName = scenarioResponse.GetScenarioName();
        if (matches.contains(scenarioName)) {
            allowedScenarios.insert(scenarioName);
        } else if (featuresMatches.contains(scenarioName)) {
            for (const auto& feature : featuresMatches.at(scenarioName)) {
                if (CheckFeatureSupport(interfaces, feature, ctx.Logger())) {
                    allowedScenarios.insert(scenarioName);
                    break;
                }
            }
        }
    }

    if (!allowedScenarios) {
        return false;
    }
    LOG_INFO(ctx.Logger()) << "Scenarios allowed by fixlist: " << JoinSeq("|", allowedScenarios);
    return TryEraseIf(responses, [&allowedScenarios](const auto& response) {
        return !allowedScenarios.contains(response.GetScenarioName());
    }, qualityStorage, LR_FILTERED_BY_FIXLIST);
}

bool LeaveOnlyRelevantPlayerOnPlayerRequests(const IContext& ctx, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage) {
    const auto responseDoesntHaveRestorePlayerFeature = [](const TScenarioResponse& response) {
        return !response.GetFeatures().GetScenarioFeatures().GetPlayerFeatures().GetRestorePlayer();
    };
    const bool someResponseHasRestorePlayer = !AllOf(responses, responseDoesntHaveRestorePlayerFeature);

    if (someResponseHasRestorePlayer) {
        TryEraseIf(responses, responseDoesntHaveRestorePlayerFeature, qualityStorage, LR_DOESNT_RESTORE_PLAYER);

        const auto getSecondsSincePause = [](const TScenarioResponse& response) {
            return response.GetFeatures().GetScenarioFeatures().GetPlayerFeatures().GetSecondsSincePause();
        };
        const auto minSecondsSincePause = getSecondsSincePause(*MinElementBy(responses, getSecondsSincePause));

        TryEraseIf(responses, [getSecondsSincePause, minSecondsSincePause](const TScenarioResponse& response) {
            return getSecondsSincePause(response) > minSecondsSincePause;
        }, qualityStorage, LR_NOT_LAST_PLAYER);

        if (responses.size() > 1) {
            LOG_WARN(ctx.Logger()) << "Multiple players have same SecondsSincePause feature!";
        }
    }
    return someResponseHasRestorePlayer;
}

bool LeaveOnlyScenariosWithPriorityGreaterThanProtocolIfExist(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    const auto hasPriorityLessOrEqualThanProtocolScenarioPriority =
        [&ctx, &scenarioRegistry, &formulasStorage](const TScenarioResponse& response) {
            const auto thisResponsePriority = GetScenarioPriority(ctx, scenarioRegistry, response.GetScenarioName(), formulasStorage);
            return thisResponsePriority <= NMegamind::MM_PROTOCOL_SCENARIO_PRIORITY;
        };

    if (AllOf(responses, hasPriorityLessOrEqualThanProtocolScenarioPriority)) {
        return false;
    }
    return TryEraseIf(responses, hasPriorityLessOrEqualThanProtocolScenarioPriority, qualityStorage, LR_EARLY_PRIORITY);
}

bool LeaveOnlyVinsOnImportantScenarios(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    const auto vinsScenarioName = GetVinsScenarioName(ctx);
    const TScenarioResponse* vinsResponse = GetScenarioResponse(responses, vinsScenarioName);
    if (!vinsResponse) {
        return false;
    }

    const auto& intent = vinsResponse->GetIntentFromFeatures();
    const auto& wizardResponse = ctx.Responses().WizardResponse();

    const auto* importantScenarios = VINS_IMPORTANT_SCENARIOS.FindPtr(ctx.LanguageForClassifiers());

    if (importantScenarios && importantScenarios->contains(intent)) {
        return LeaveOnlyWithPriority(vinsScenarioName, ctx, scenarioRegistry, LR_VINS_IMPORTANT_SCENARIOS, formulasStorage, responses, qualityStorage);
    }

    const auto* importantPrefixes = VINS_IMPORTANT_SCENARIOS_PREFIXES.FindPtr(ctx.LanguageForClassifiers());
    const auto* exceptionsFromImportant = VINS_IMPORTANT_SCENARIOS_EXCEPTIONS.FindPtr(ctx.LanguageForClassifiers());

    if (importantPrefixes && !(exceptionsFromImportant && exceptionsFromImportant->contains(intent))) {
        for (const auto& prefix : *importantPrefixes) {
            if (intent.StartsWith(prefix)) {
                return LeaveOnlyWithPriority(vinsScenarioName, ctx, scenarioRegistry, LR_VINS_IMPORTANT_SCENARIOS, formulasStorage, responses, qualityStorage);
            }
        }
    }

    const auto* importantPrefixesAndGrammars = VINS_IMPORTANT_SCENARIOS_PREFIXES_AND_GRAMMARS.FindPtr(ctx.LanguageForClassifiers());

    if (importantPrefixesAndGrammars) {
        for (const auto& condition : *importantPrefixesAndGrammars) {
            if (intent.StartsWith(condition.IntentPrefix) && condition.CheckEnabled(ctx)) {
                for (const auto grammarName : condition.Grammars) {
                    if (wizardResponse.HasGranetFrame(grammarName)) {
                        return LeaveOnlyWithPriority(vinsScenarioName, ctx, scenarioRegistry, LR_VINS_IMPORTANT_SCENARIOS_AND_GRAMMARS, formulasStorage, responses, qualityStorage);
                    }
                }
            }
        }
    }

    return false;
}

bool LeaveOnlyVideoOnImportantScenarios(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TVector<TSemanticFrame>& recognizedActionEffectFrames,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    const TScenarioResponse* videoResponse = GetScenarioResponse(responses, MM_VIDEO_PROTOCOL_SCENARIO);
    if (!videoResponse) {
        return false;
    }

    const auto& intent = videoResponse->GetIntentFromFeatures();
    if (intent.Empty()) {
        return false;
    }
    LOG_INFO(ctx.Logger()) << "actual video intent: " << intent;
    const auto& wizardResponse = ctx.Responses().WizardResponse();

    const auto* videoImportantPrefixesAndGrammars = VIDEO_IMPORTANT_SCENARIOS_PREFIXES_AND_GRAMMARS.FindPtr(ctx.LanguageForClassifiers());

    if (videoImportantPrefixesAndGrammars) {
        for (const auto& [intentPrefix, grammarNames] : *videoImportantPrefixesAndGrammars) {
            if (intent.StartsWith(intentPrefix)) {
                for (const auto grammarName : grammarNames) {
                    LOG_INFO(ctx.Logger()) << "checking grammar: " << grammarName;
                    if (!!wizardResponse.GetRequestFrame(grammarName)) {
                        if (ctx.HasExpFlag(EXP_VIDEO_FORCE_ITEM_SELECTION_INSTEAD_OF_TV_CHANNELS) &&
                            intentPrefix == "mm.personal_assistant.scenarios.quasar.select_video_from_gallery" && // https://st.yandex-team.ru/SMARTTVBACKEND-1121
                            TryEraseIf(responses, [](const auto& response) {
                                return response.GetScenarioName() == MM_TV_CHANNELS_SCENARIO;
                            }, qualityStorage, LR_VIDEO_IMPORTANT_SCENARIOS))
                        {
                            LOG_INFO(ctx.Logger()) << "Erase " << MM_TV_CHANNELS_SCENARIO << " when " << intentPrefix << " was choosen in " << MM_VIDEO_PROTOCOL_SCENARIO;
                        }

                        return LeaveOnlyWithPriority(MM_VIDEO_PROTOCOL_SCENARIO, ctx, scenarioRegistry, LR_VIDEO_IMPORTANT_SCENARIOS, formulasStorage, responses, qualityStorage);
                    }
                }
            }
        }
    }

    if (ctx.HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)) {
        for (const auto& recognizedActionEffectFrame : recognizedActionEffectFrames) {
            if (recognizedActionEffectFrame.GetName() == MM_VIDEO_PROTOCOL_SCENARIO) {
                return LeaveOnlyWithPriority(MM_VIDEO_PROTOCOL_SCENARIO, ctx, scenarioRegistry, LR_VIDEO_GC_PROACTIVITY, formulasStorage, responses, qualityStorage);
            }
        }
    }
    return false;
}

bool LeaveOnlyBoostedScenariosOnImportantFrames(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    const auto& wizardResponse = ctx.Responses().WizardResponse();

    const auto* importantFrames = SCENARIO_IMPORTANT_FRAMES.FindPtr(ctx.LanguageForClassifiers());
    if (importantFrames == nullptr) {
        return false;
    }

    for (const auto& response : responses) {
        const auto range = importantFrames->equal_range(response.GetScenarioName());
        for (auto it = range.first; it != range.second; ++it) {
            const auto& condition = it->second;
            if (wizardResponse.HasGranetFrame(condition.Frame) && condition.CheckEnabled(ctx) &&
                response.GetIntentFromFeatures().StartsWith(condition.IntentPrefix))
            {
                return LeaveOnlyWithPriorityAbovePostclassify(response.GetScenarioName(), ctx, scenarioRegistry, LR_FRAME_BOOSTING, formulasStorage, responses, qualityStorage);
            }
        }
    }

    return false;
}

bool LeaveOnlyScenariosPreferableToVinsIntentsAfterFormula(const IContext& ctx, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage) {
    const auto vinsScenarioName = GetVinsScenarioName(ctx);

    auto vinsResponse = FindIf(responses, [name=vinsScenarioName](const auto& response) {
        return response.GetScenarioName() == name;
    });
    if (vinsResponse == responses.end() || vinsResponse != responses.begin()) {
        return false;
    }
    const auto& vinsIntent = vinsResponse->GetIntentFromFeatures();

    const auto* preferableScenarioToVinsIntentAfterFormula = PREFERABLE_SCENARIO_TO_VINS_INTENT_AFTER_FORMULA.FindPtr(ctx.LanguageForClassifiers());

    if (preferableScenarioToVinsIntentAfterFormula == nullptr) {
        return false;
    }

    for (const auto& [vinsIntentToSwap, replacementScenario] : *preferableScenarioToVinsIntentAfterFormula) {
        if (vinsIntent != vinsIntentToSwap) {
            continue;
        }
        if (!replacementScenario.CheckEnabled(ctx)) {
            continue;
        }
        auto scenarioResponse = FindIf(
            responses,
            [name=replacementScenario.ScenarioName, frameName=replacementScenario.ScenarioFrameName](const auto& response) {
                return response.GetScenarioName() == name && (!frameName || response.GetResponseSemanticFrame().GetName() == frameName);
            });
        if (scenarioResponse == responses.end()) {
            continue;
        }
        return LeaveOnly(replacementScenario.ScenarioName, responses, qualityStorage, LR_SWAPPED_WITH_PREFERABLE);
    }
    return false;
}

bool LeaveOnlyVideoOnHollywoodMusicForTv(const TClientFeatures& clientFeatures, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage) {
    if (clientFeatures.SupportsMusicQuasarClient() || !clientFeatures.IsTvDevice() || responses.size() < 2) {
        return false;
    }
    auto winnerResponse = responses.begin();
    if (winnerResponse->GetScenarioName() != HOLLYWOOD_MUSIC_SCENARIO) {
        return false;
    }
    auto videoResponse = FindIf(responses, [](const auto& response) {
        return response.GetScenarioName() == MM_VIDEO_PROTOCOL_SCENARIO;
    });
    if (videoResponse == responses.end()) {
        return false;
    }

    for (const TSemanticFrame& frame : winnerResponse->GetRequestSemanticFrames()) {
        if (frame.GetName() == NMusic::MUSIC_PLAY) {
            const auto searchTextSlot = FindIfPtr(frame.GetSlots(), [](const auto& slot) {
                return slot.GetName() == NMusic::SLOT_SEARCH_TEXT;
            });
            if (searchTextSlot) {
                return LeaveOnly(MM_VIDEO_PROTOCOL_SCENARIO, responses, qualityStorage, LR_SWAPPED_WITH_PREFERABLE);
            }
            break;
        }
    }
    return false;
}

bool LeaveOnlyMusicOnImportantScenarios(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TVector<TSemanticFrame>& recognizedActionEffectFrames,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    const TScenarioResponse* hollywoodMusicResponse = GetScenarioResponse(responses, HOLLYWOOD_MUSIC_SCENARIO);
    if (!hollywoodMusicResponse) {
        return false;
    }

    if (ctx.HasExpFlag(EXP_ENABLE_GC_PROACTIVITY)) {
        for (const auto& recognizedActionEffectFrame : recognizedActionEffectFrames) {
            if (recognizedActionEffectFrame.GetName() == NMusic::MUSIC_PLAY) {
                return LeaveOnlyWithPriority(HOLLYWOOD_MUSIC_SCENARIO, ctx, scenarioRegistry, LR_MUSIC_GC_PROACTIVITY, formulasStorage, responses, qualityStorage);
            }
        }
    }
    return false;
}

bool LeaveOnlyHollywoodMusicOnPlayerIntents(
    const IContext& ctx,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    const TScenarioResponse* hollywoodMusicResponse = GetScenarioResponse(responses, HOLLYWOOD_MUSIC_SCENARIO);
    if (hollywoodMusicResponse) {
        if (hollywoodMusicResponse->GetFeatures().GetScenarioFeatures().GetMusicFeatures().GetIsPlayerCommand()) {
            return LeaveOnlyWithPriority(HOLLYWOOD_MUSIC_SCENARIO, ctx, scenarioRegistry, LR_NOT_PLAYER_OWNER, formulasStorage, responses, qualityStorage);
        }
    }
    return false;
}

const THashSet<TStringBuf> POSTCLASSIFIER_GC_FORCE_INTENTS = {
    TStringBuf("alice.generative_tale"),
    TStringBuf("alice.generative_tale.activate"),
    TStringBuf("alice.generative_tale.send_me_my_tale"),
    TStringBuf("alice.generative_toast"),
};

bool LeaveOnlyGeneralConversationOnImportantScenarios(const IContext& ctx, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage) {
    const auto allowedIntentsFromExpString = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_POSTCLASSIFIER_GC_FORCE_INTENTS_PREFIX);
    if (!allowedIntentsFromExpString && POSTCLASSIFIER_GC_FORCE_INTENTS.empty()) {
        return false;
    }
    const TScenarioResponse* gcResponse = GetScenarioResponse(responses, PROTOCOL_GENERAL_CONVERSATION_SCENARIO);
    if (!gcResponse) {
        return false;
    }
    const auto& intent = gcResponse->GetIntentFromFeatures();
    if (intent.Empty()) {
        return false;
    }

    if (POSTCLASSIFIER_GC_FORCE_INTENTS.contains(intent) ||
        (allowedIntentsFromExpString && THashSet<TStringBuf>(StringSplitter(allowedIntentsFromExpString.GetRef()).Split(';')).contains(intent))) {
        return LeaveOnly(PROTOCOL_GENERAL_CONVERSATION_SCENARIO, responses, qualityStorage, LR_GC_IMPORTANT_SCENARIOS);
    }

    return false;
}

bool LeaveOnlyVinsScenarioByPreferableIntents(const IContext& ctx, const TScenarioConfigRegistry& scenarioRegistry,
                                              const TFormulasStorage& formulasStorage, TVector<TScenarioResponse>& responses,
                                              TQualityStorage& qualityStorage) {
    const auto vinsScenarioName = GetVinsScenarioName(ctx);
    auto vinsResponse = FindIf(responses, [name=vinsScenarioName](const auto& response) {
        return response.GetScenarioName() == name;
    });
    if (vinsResponse == responses.end()) {
        return false;
    }

    const auto& vinsIntent = vinsResponse->GetIntentFromFeatures();
    const auto* preferableIntents = PREFERABLE_INTENTS_IN_VINS.FindPtr(ctx.LanguageForClassifiers());

    if (preferableIntents && preferableIntents->contains(vinsIntent)) {
        return TryEraseIf(responses, [&vinsScenarioName, &ctx, &scenarioRegistry, &formulasStorage](const auto& response) {
            const auto& name = response.GetScenarioName();
            return name != vinsScenarioName &&
                GetScenarioPriority(ctx, scenarioRegistry, name, formulasStorage) < NMegamind::MM_BOOSTED_VINS_SCENARIO_PRIORITY;
        },
        qualityStorage,
        LR_VINS_IMPORTANT_SCENARIOS);
    }

    return false;
}

bool CanLeaveOnlySideSpeech(
    const IContext& ctx,
    const THashMap<TString, double>& postclassifierPredicts,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses
) {
    auto sideSpeechResponse =  FindIf(responses, [](const TScenarioResponse& response) {
        return response.GetScenarioName() == SIDE_SPEECH_SCENARIO;
    });
    if (sideSpeechResponse == responses.end()) {
        return false;
    }

    const auto* predict = postclassifierPredicts.FindPtr(SIDE_SPEECH_SCENARIO);
    if (!predict || ctx.HasExpFlag(EXP_DISABLE_SIDE_SPEECH_CLASSIFIER)) {
        return false;
    }
    const auto clientType = GetClientType(ctx.ClientFeatures());
    const auto threshold = NMegamind::GetExperimentalSideSpeechThreshold(
        ctx.ExpFlags()
    ).GetOrElse(
        NMegamind::GetScenarioConfidentThreshold(
            formulasStorage,
            SIDE_SPEECH_SCENARIO,
            ECS_POST,
            clientType,
            ctx.ExpFlags(),
            ctx.ClassificationConfig(),
            ctx.Logger(),
            ctx.LanguageForClassifiers()
        )
    );
    return *predict > threshold;
}

bool LeaveOnlySideSpeech(
    const IContext& ctx,
    const THashMap<TString, double>& postclassifierPredicts,
    const TFormulasStorage& formulasStorage,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    if (CanLeaveOnlySideSpeech(ctx, postclassifierPredicts, formulasStorage, responses)) {
        return LeaveOnly(SIDE_SPEECH_SCENARIO, responses, qualityStorage, LR_CUT_BY_SIDESPEECH_FORMULA);
    }
    return false;
}

bool TryForceSlotRequestResponse(const IContext& ctx, TVector<TScenarioResponse>& responses, TQualityStorage& qualityStorage) {
    if (!ctx.Session()) {
        return false;
    }
    const ISession& session = *ctx.Session();

    const TMaybe<TSemanticFrame::TSlot> requestedSlot = session.GetRequestedSlot();

    if (!requestedSlot.Defined()) {
        return false;
    }

    TScenarioResponse* slotRequestResponse = nullptr;
    bool hasOtherStronglyRelevantResponse = false;
    for (auto& response : responses) {
        if (response.GetScenarioName() == session.GetPreviousScenarioName()) {
            slotRequestResponse = &response;
        } else if (!response.GetFeatures().GetScenarioFeatures().GetIsIrrelevant() &&
                   !response.GetScenarioAcceptsAnyUtterance())
        {
            hasOtherStronglyRelevantResponse = true;
        }
    }
    if (!slotRequestResponse) {
        return false;
    }

    const auto prevResponseFrame = session.GetResponseFrame();
    if (!prevResponseFrame.Defined()) {
        return false;
    }

    const auto filledRequestedSlot = GetFilledRequestedSlot(
        slotRequestResponse->GetRequestSemanticFrames(),
        *prevResponseFrame
    );

    // FIXME(the0, DIALOG-6249): Generally speaking, there may be a case when
    // we get several semantic frames with same name and even same slot filled
    // which is by chance a previously requested one. If there are both frames
    // with typed and untyped requested value options in a request we should examine
    // those frames carefully, with special respect to typed slots.

    const bool hasRelatedFrame = filledRequestedSlot.Defined();
    const bool isFilledRequestedSlotUntyped =
        filledRequestedSlot.Defined() && filledRequestedSlot->GetType() == "string";

    bool isIrrelevant = slotRequestResponse->GetFeatures().GetScenarioFeatures().GetIsIrrelevant();
    bool hasNiceResponse = false;
    if (!hasRelatedFrame && !isIrrelevant) {
        isIrrelevant = true;
        hasNiceResponse = true;
    }

    if (isIrrelevant) {
        if (hasOtherStronglyRelevantResponse) {
            return false;
        }

        if (!hasNiceResponse) {
            return false;
        }

        if (!ShouldForceResponseForEmptyRequestedSlot(ctx, slotRequestResponse->GetScenarioName())) {
            return false;
        }

        return TryEraseIf(
            responses,
            [name=slotRequestResponse->GetScenarioName()](const TScenarioResponse& response) {
                return response.GetScenarioName() != name;
            },
            qualityStorage, LR_REQUESTED_FRAME
        );
    }

    if (!hasRelatedFrame && hasOtherStronglyRelevantResponse) {
        return false;
    }

    if (isFilledRequestedSlotUntyped &&
        !ShouldForceResponseForFilledUntypedRequestedSlot(ctx, slotRequestResponse->GetScenarioName())
    ) {
        return false;
    }

    return TryEraseIf(
        responses,
        [name=slotRequestResponse->GetScenarioName()](const TScenarioResponse& response) {
            return response.GetScenarioName() != name;
        },
        qualityStorage,
        LR_REQUESTED_FRAME
    );
}

bool ProcessActions(
    const TVector<TSemanticFrame>& recognizedActionEffectFrames,
    TVector<TScenarioResponse>& responses,
    TQualityStorage& qualityStorage
) {
    if (recognizedActionEffectFrames.empty()) {
        return false;
    }
    const auto doesntAcceptActionFrame = [&recognizedActionEffectFrames](const TScenarioResponse& r) {
        for (const auto& recognizedActionEffectFrame : recognizedActionEffectFrames) {
            for (const TSemanticFrame& frame : r.GetRequestSemanticFrames()) {
                if (frame.GetName() == recognizedActionEffectFrame.GetName()) {
                    return false;
                }
            }
        }
        return true;
    };

    if (!AllOf(responses, doesntAcceptActionFrame)) {
        return TryEraseIf(responses, doesntAcceptActionFrame, qualityStorage, LR_ACTION_NOT_SUPPORTED);
    }
    return false;
}

TString LogScenarios(const TVector<TScenarioResponse>& scenarioResponses) {
    if (scenarioResponses.empty()) {
        return TString{"-"};
    }

    auto range = MakeMappedRange(
        scenarioResponses,
        [](const TScenarioResponse& response) {
            return response.GetScenarioName();
        });
    return JoinRange(/* delim= */ TStringBuf(", "), range.begin(), range.end());
}

} // namespace

void PostClassify(
    const IContext& ctx,
    const NScenarios::TInterfaces& interfaces,
    const TMaybe<TRequest::TScenarioInfo>& boostedScenario,
    const TVector<TSemanticFrame>& recognizedActionEffectFrames,
    const TScenarioConfigRegistry& scenarioRegistry,
    const TFormulasStorage& formulasStorage,
    const TFactorStorage& factorStorage,
    TVector<TScenarioResponse>& scenarioResponses,
    TQualityStorage& qualityStorage
) {
    LOG_INFO(ctx.Logger()) << TLogMessageTag{"PostClassification stage"} << "PostClassifiy: " << LogScenarios(scenarioResponses);
    if (boostedScenario.Defined()) {
        LeaveOnly(boostedScenario->GetName(), scenarioResponses, qualityStorage, LR_BOOSTED_SCENARIO);
        LOG_INFO(ctx.Logger()) << "LeaveOnlyBoostedScenario(" << boostedScenario->GetName() << "): "
            << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_BOOSTED);
    }

    const auto forcedScenarioName = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_PREFIX_MM_FORCE_SCENARIO);
    if (forcedScenarioName.Defined()) { // Prevent erasing of forced scenario if it's priority wasn't set (or negative)
        LeaveOnly(forcedScenarioName.GetRef(), scenarioResponses, qualityStorage, LR_FORCED_SCENARIO);
        LOG_INFO(ctx.Logger()) << "LeaveOnlyForcedScenario(" << forcedScenarioName.GetRef() << "): "
            << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_FORCED);
    }

    const auto* vinsDependentScenarios = VINS_DEPENDENT_SCENARIOS.FindPtr(ctx.LanguageForClassifiers());

    if (vinsDependentScenarios && ctx.HasExpFlag(EXP_FORCE_VINS_SCENARIOS) && TryEraseIf(
            scenarioResponses,
            [&vinsDependentScenarios](const TScenarioResponse& response) { return !IsIn(*vinsDependentScenarios, response.GetScenarioName()); },
            qualityStorage,
            LR_FORCED_SCENARIO
        ))
    {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyVinsDependentScenarios: " << LogScenarios(scenarioResponses);
    }

    const auto hasNegativePriority = [&](const TScenarioResponse& response) {
        return GetScenarioPriority(ctx, scenarioRegistry, response.GetScenarioName(), formulasStorage) < 0;
    };
    if (TryEraseIf(scenarioResponses, hasNegativePriority, qualityStorage, LR_NEGATIVE_PRIORITY)) {
        LOG_INFO(ctx.Logger()) << "EraseIfNegativePriority: " << LogScenarios(scenarioResponses);
    }

    if (TryForceSlotRequestResponse(ctx, scenarioResponses, qualityStorage)) {
        qualityStorage.SetPostclassificationWinReason(WR_REQUESTED_SLOT);
        LOG_INFO(ctx.Logger()) << "ForceSlotRequestResponse: " << LogScenarios(scenarioResponses);
    }

    THashMap<TString, double> postclassifierPredicts;
    const auto clientType = GetClientType(ctx.ClientFeatures());
    const TMaybe<TStringBuf> experiment = NMegamind::GetMMFormulaExperimentForSpecificClient(ctx.ExpFlags(), clientType);

    auto calcPostPredict = [&postclassifierPredicts, &formulasStorage, &clientType,
                            &experiment, &factorStorage, &ctx,
                            &qualityStorage](const TString& scenarioName) {
        if (postclassifierPredicts.contains(scenarioName)) {
            return;
        }
        const double predict = ApplyScenarioFormula(
            formulasStorage, scenarioName, ECS_POST, clientType,
            experiment.GetOrElse({}), factorStorage, ctx.Logger(), ctx.LanguageForClassifiers());

        auto& storagePredicts = *qualityStorage.MutablePostclassifierPredicts();
        storagePredicts[scenarioName] = predict;
        postclassifierPredicts[scenarioName] = predict;
    };

    calcPostPredict(TString{SIDE_SPEECH_SCENARIO});

    auto isIrrelevant =
        [&ctx, &postclassifierPredicts, &formulasStorage, &scenarioResponses](const TScenarioResponse& response) {
            if (response.GetScenarioName() == SIDE_SPEECH_SCENARIO) {
                return response.GetFeatures().GetScenarioFeatures().GetIsIrrelevant() ||
                       !CanLeaveOnlySideSpeech(ctx, postclassifierPredicts, formulasStorage, scenarioResponses);
            }
            return response.GetFeatures().GetScenarioFeatures().GetIsIrrelevant();
        };

    // stores an irrelevant VINS response so that the swap trick can work with irrelevant intents
    TMaybe<TScenarioResponse> irrelevantVinsResponse;
    bool erasedIrrelevantVins = false;

    if (!AllOf(scenarioResponses, isIrrelevant)) {

        if (ctx.LanguageForClassifiers() == LANG_RUS) {
            // swap trick is needed only for russian stack
            const auto vinsIt = std::find_if(
                scenarioResponses.begin(), scenarioResponses.end(),
                [vinsName = GetVinsScenarioName(ctx)](const TScenarioResponse& scenarioResponse) {
                    return scenarioResponse.GetScenarioName() == vinsName;
                }
            );
            if (vinsIt != scenarioResponses.end() && isIrrelevant(*vinsIt)) {
                erasedIrrelevantVins = true;
                LOG_INFO(ctx.Logger()) << "There is an irrelevant VINS response, saving it for later";
                irrelevantVinsResponse = std::move(*vinsIt);
                scenarioResponses.erase(vinsIt);
            }
        }

        if (TryEraseIf(scenarioResponses, isIrrelevant, qualityStorage, LR_IRRELEVANT)) {
            LOG_INFO(ctx.Logger()) << "EraseIrrelevant: " << LogScenarios(scenarioResponses);
        }
    }

    if (ProcessActions(recognizedActionEffectFrames, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "ProcessActions: " << LogScenarios(scenarioResponses);
    }

    const auto isInModalMode = [&ctx](const TScenarioResponse& response) {
        return IsInModalMode(ctx, response.GetScenarioName()) &&
               !response.GetFeatures().GetScenarioFeatures().GetIgnoresExpectedRequest();
    };
    if (const auto* modalScenario = FindIfPtr(scenarioResponses, isInModalMode)) {
        LeaveOnly(modalScenario->GetScenarioName(), scenarioResponses, qualityStorage, LR_MODAL_SCENARIO);
        qualityStorage.SetPostclassificationWinReason(WR_MODAL);
        LOG_INFO(ctx.Logger()) << "LeaveOnlyModalScenario: " << LogScenarios(scenarioResponses);
    }

    const auto vinsScenarioName = GetVinsScenarioName(ctx);
    if (ctx.LanguageForClassifiers() == LANG_RUS && LeaveSkillDiscoveryOnlyOnGcIntent(scenarioResponses, vinsScenarioName, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveSkillDiscoveryOnlyOnGcIntent: " << LogScenarios(scenarioResponses);
    }

    const auto isNotContinuing = [](const TScenarioResponse& response) {
        return !response.GetFeatures().GetScenarioFeatures().GetVinsFeatures().GetIsContinuing();
    };
    if (!AllOf(scenarioResponses, isNotContinuing)) {
        auto numberOfScenariosErased = scenarioResponses.size();
        TryEraseIf(scenarioResponses, isNotContinuing, qualityStorage, LR_IS_CONTINUING);
        numberOfScenariosErased -= scenarioResponses.size();
        if (numberOfScenariosErased) {
            LOG_INFO(ctx.Logger()) << "Erased " << numberOfScenariosErased << " scenarios due IsContinuing flag";
        }
        LOG_INFO(ctx.Logger()) << "EraseIsNotContinuing: " << LogScenarios(scenarioResponses);
    }

    if (LeaveOnlyScenariosWithPriorityGreaterThanProtocolIfExist(ctx, scenarioRegistry, formulasStorage, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyScenariosWithPriorityGreaterThanProtocolIfExist: " << LogScenarios(scenarioResponses);
    }

    // here the fixlist (AliceFixlist rule) is applied only to the high-level scenarios
    // low-level intents will be fixlisted in a scenario-specific way (e.g. in VINS)
    if (FilterScenariosByFixlist(ctx, interfaces, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "FilterScenariosByFixlist: " << LogScenarios(scenarioResponses);
    }
    if (!ctx.HasExpFlag(EXP_DISABLE_PLAYER_FEATURES) && LeaveOnlyRelevantPlayerOnPlayerRequests(ctx, scenarioResponses, qualityStorage)) {
        qualityStorage.SetPostclassificationWinReason(WR_RESTORE_PLAYER);
        LOG_INFO(ctx.Logger()) << "LeaveOnlyRelevantPlayerOnPlayerRequests: " << LogScenarios(scenarioResponses);
    }
    if (LeaveOnlyVinsOnImportantScenarios(ctx, scenarioRegistry, formulasStorage, scenarioResponses, qualityStorage)) {
        qualityStorage.SetPostclassificationWinReason(WR_IMPORTANT_INTENTS);
        LOG_INFO(ctx.Logger()) << "LeaveOnlyVinsOnImportantScenarios: " << LogScenarios(scenarioResponses);
    }
    if (LeaveOnlyVideoOnImportantScenarios(ctx, scenarioRegistry, recognizedActionEffectFrames, formulasStorage, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyVideoOnImportantScenarios(" << MM_VIDEO_PROTOCOL_SCENARIO
            << "): " << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_IMPORTANT_INTENTS);
    }
    if (LeaveOnlyMusicOnImportantScenarios(ctx, scenarioRegistry, recognizedActionEffectFrames, formulasStorage, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyMusicOnImportantScenarios: " << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_IMPORTANT_INTENTS);
    }
    if (LeaveOnlyGeneralConversationOnImportantScenarios(ctx, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyGeneralConversationOnImportantScenarios: " << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_IMPORTANT_INTENTS);
    }
    if (LeaveOnlyBoostedScenariosOnImportantFrames(ctx, scenarioRegistry, formulasStorage, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyBoostedScenariosOnImportantFrames: " << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_IMPORTANT_INTENTS);
    }
    if (LeaveOnlyVinsScenarioByPreferableIntents(ctx, scenarioRegistry, formulasStorage, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyVinsScenarioByPreferableIntents: " << LogScenarios(scenarioResponses);
    }
    if (ctx.HasExpFlag(EXP_DISABLE_PLAYER_FEATURES) &&
        LeaveOnlyHollywoodMusicOnPlayerIntents(ctx, scenarioRegistry, formulasStorage, scenarioResponses, qualityStorage))
    {
        LOG_INFO(ctx.Logger()) << "LeaveOnlyHollywoodMusicOnPlayerIntents: " << LogScenarios(scenarioResponses);
    }

    if (irrelevantVinsResponse) {
        scenarioResponses.push_back(std::move(*irrelevantVinsResponse));
        irrelevantVinsResponse = Nothing();
        LOG_INFO(ctx.Logger()) << "ReinsertIrrelevantVins: " << LogScenarios(scenarioResponses);
    }

    for (const TScenarioResponse& scenarioResponse : scenarioResponses) {
        const TString& name = scenarioResponse.GetScenarioName();
        calcPostPredict(name);
    }

    const auto priorityGetter = [&ctx, &scenarioRegistry, &formulasStorage](const TScenarioResponse& response) {
        return GetScenarioPriority(ctx, scenarioRegistry, response.GetScenarioName(), formulasStorage);
    };

    const auto languageGetter = [&ctx, &scenarioRegistry](const TScenarioResponse& response) {
        return NMegamind::GetScenarioRequestLanguage(scenarioRegistry.GetScenarioConfig(response.GetScenarioName()), ctx);
    };

    RankByPriorityAndPredicts(scenarioResponses, priorityGetter, languageGetter, postclassifierPredicts, ctx.LanguageForClassifiers());
    LOG_INFO(ctx.Logger()) << "RankByPriorityAndPredicts: " << LogScenarios(scenarioResponses);

    if (LeaveOnlySideSpeech(ctx, postclassifierPredicts, formulasStorage, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "LeaveOnlySideSpeech: " << LogScenarios(scenarioResponses);
    }

    if (LeaveOnlyScenariosPreferableToVinsIntentsAfterFormula(ctx, scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "SwapScenariosPreferableToVinsIntents: " << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_SWAPPED_AFTER_FORMULA);
    }

    if (erasedIrrelevantVins) {
        if (!TryEraseIf(scenarioResponses, isIrrelevant, qualityStorage, LR_IRRELEVANT)) {
            LOG_WARNING(ctx.Logger()) << "Expected to erase irrelevantVins but erased nothing";
        }
        LOG_INFO(ctx.Logger()) << "EraseIrrelevantVins: " << LogScenarios(scenarioResponses);
    }

    if (LeaveOnlyVideoOnHollywoodMusicForTv(ctx.ClientFeatures(), scenarioResponses, qualityStorage)) {
        LOG_INFO(ctx.Logger()) << "SwapHollywoodMusicWithVideoForTv: " << LogScenarios(scenarioResponses);
        qualityStorage.SetPostclassificationWinReason(WR_SWAPPED_AFTER_FORMULA);
    }

    FillScenariosWinLossReasons(scenarioResponses, ctx, scenarioRegistry, formulasStorage, qualityStorage);

    if (!scenarioResponses.empty()) {
        const auto& winner = scenarioResponses.front();
        const TString tag = TString{"Winner scenario: "} + winner.GetScenarioName();
        LOG_INFO(ctx.Logger()) << TLogMessageTag{tag} << tag;
        ctx.Sensors().IncRate(NSignal::LabelsForWinningScenario(winner.GetScenarioName()));
    }
}

} // namespace NAlice
