#include "session_start.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/music/music.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/vector.h>
#include <alice/bass/forms/external_skill_recommendation/enums.h>
#include <alice/bass/forms/external_skill_recommendation/skill_recommendation.h>

namespace {

constexpr TStringBuf SessionStartFormName = "personal_assistant.internal.session_start";
constexpr TStringBuf OnboardingFormName = "personal_assistant.scenarios.what_can_you_do";

// Suggest groups, random one will be returned.
const TVector<TVector<TStringBuf>> SUGGESTS = {
    {
        TStringBuf("onboarding__get_weather__now"),
        TStringBuf("onboarding__search_internet__open_vk"),
        TStringBuf("onboarding__find_poi__pharmacy_nearby"),
        TStringBuf("onboarding__find_poi__barbershop"),
        TStringBuf("onboarding__search_internet__kozlovsky_height"),
        TStringBuf("onboarding__convert__dollar_rate_today"),
        TStringBuf("onboarding__handcrafted__what_is_your_name")
    },
    {
        TStringBuf("onboarding__get_weather__tomorrow"),
        TStringBuf("onboarding__search_internet__open_odnoklassniki"),
        TStringBuf("onboarding__find_poi__cafe_nearby"),
        TStringBuf("onboarding__find_poi__carwash_nearby"),
        TStringBuf("onboarding__search_internet__pushkin_dates"),
        TStringBuf("onboarding__convert__euro_rate_today"),
        TStringBuf("onboarding__handcrafted__tell_me_a_tale")
    },
    {
        TStringBuf("onboarding__get_weather__weekend"),
        TStringBuf("onboarding__search_internet__open_facebook"),
        TStringBuf("onboarding__find_poi__restaurant_nearby"),
        TStringBuf("onboarding__find_poi__cinema"),
        TStringBuf("onboarding__search_internet__summer_duration"),
        TStringBuf("onboarding__convert__hundred_dollar_in_roubles"),
        TStringBuf("onboarding__handcrafted__life_on_mars")
    },
    {
        TStringBuf("onboarding__get_weather__couple_days"),
        TStringBuf("onboarding__search_internet__yamaps"),
        TStringBuf("onboarding__find_poi__dinner"),
        TStringBuf("onboarding__find_poi__pharmacy_nearby"),
        TStringBuf("onboarding__search_internet__termodynamics_second_law"),
        TStringBuf("onboarding__convert__hundred_euros_in_roubles"),
        TStringBuf("onboarding__handcrafted__who_are_your_friends")
    },
    {
        TStringBuf("onboarding__get_weather__today"),
        TStringBuf("onboarding__search_internet__open_vk"),
        TStringBuf("onboarding__find_poi__coffee_nearby"),
        TStringBuf("onboarding__find_poi__flowershop_nearby"),
        TStringBuf("onboarding__search_internet__robotlaws_whatis"),
        TStringBuf("onboarding__convert__euro_price_today"),
        TStringBuf("onboarding__handcrafted__what_is_love")
    },
};

const TVector<TVector<TStringBuf>> NAVIGATOR_SUGGESTS = {
    {
        TStringBuf("onboarding__navi__go"),
        TStringBuf("onboarding__navi__find_poi__gasoline"),
        TStringBuf("onboarding__navi__go__home"),
        TStringBuf("onboarding__navi__set__accident"),
        TStringBuf("onboarding__navi__enable_traffic"),
        TStringBuf("onboarding__navi__play__city"),
        TStringBuf("onboarding__navi__weather__tomorrow")
    },
    {
        TStringBuf("onboarding__navi__play__believe"),
        TStringBuf("onboarding__navi__go__work"),
        TStringBuf("onboarding__navi__set__accident"),
        TStringBuf("onboarding__navi__read__news"),
        TStringBuf("onboarding__navi__traffic__for_long"),
        TStringBuf("onboarding__navi__show__parking"),
        TStringBuf("onboarding__navi__currency__dollar")
    },
    {
        TStringBuf("onboarding__navi__set__conversation"),
        TStringBuf("onboarding__navi__enable_traffic"),
        TStringBuf("onboarding__navi__find_poi__parking"),
        TStringBuf("onboarding__navi__play__city"),
        TStringBuf("onboarding__navi__weather__tomorrow"),
        TStringBuf("onboarding__navi__currency__dollar"),
        TStringBuf("onboarding__navi__traffic__city")
    },
    {
        TStringBuf("onboarding__navi__talk"),
        TStringBuf("onboarding__navi__traffic__for_long"),
        TStringBuf("onboarding__navi__set__speed_camera"),
        TStringBuf("onboarding__navi__go__home"),
        TStringBuf("onboarding__navi__find_poi__gasoline_nearby"),
        TStringBuf("onboarding__navi__go__home_via_pharmacy"),
        TStringBuf("onboarding__navi__show__all_route")
    },
    {
        TStringBuf("onboarding__navi__traffic__for_long"),
        TStringBuf("onboarding__navi__currency__euro"),
        TStringBuf("onboarding__navi__weather__tomorrow"),
        TStringBuf("onboarding__navi__go__work"),
        TStringBuf("onboarding__navi__disable_traffic"),
        TStringBuf("onboarding__navi__find_poi__parking_nearby"),
        TStringBuf("onboarding__navi__reset_route")
    },
};

const TVector<TVector<TStringBuf>> YAMUSIC_SUGGESTS = {
    {
        TStringBuf("onboarding__music__play_artist"),
        TStringBuf("onboarding__music__play_genre"),
        TStringBuf("onboarding__music__play_activity"),
        TStringBuf("onboarding__music__what_is_playing"),
        TStringBuf("onboarding__music__like"),
        TStringBuf("onboarding__music__personal")
    },
};

const TVector<TVector<TStringBuf>> SDG_SUGGESTS = {
    {
        TStringBuf("onboarding__sdg__start_ride"),
        TStringBuf("onboarding__sdg__call_support"),
        TStringBuf("onboarding__sdg__show_route")
    },
};

} // namespace

namespace NBASS {

namespace {
const TVector<TVector<TStringBuf>>& GetSuggests(TContext& context) {
    if (context.ClientFeatures().SupportsNavigator()) {
        return NAVIGATOR_SUGGESTS;
    } else if (context.MetaClientInfo().IsYaMusic()) {
        return YAMUSIC_SUGGESTS;
    } else if (context.MetaClientInfo().IsSdg()) {
        return SDG_SUGGESTS;
    } else {
        return SUGGESTS;
    }
}
} // end anon namespace

namespace NAutomotive {

// screen state
constexpr TStringBuf MAIN_SS = "main";
constexpr TStringBuf NAVI_SS = "navi";
constexpr TStringBuf MUSIC_SS = "music";
// audio source
constexpr TStringBuf YA_MUSIC_S = "ya_music";
constexpr TStringBuf YA_RADIO_S = "ya_radio";
constexpr TStringBuf FM_S = "fm";
constexpr TStringBuf BT_S = "bluetooth";
constexpr TStringBuf AUX_S = "aux";

const TVector<TStringBuf> GO_TO = {
    TStringBuf("onboarding__navi__go__home"),
    TStringBuf("onboarding__navi__go__work"),
    TStringBuf("onboarding__automotive__lev_tolstoy")
};

const TVector<TStringBuf> RADIO = {
    TStringBuf("onboarding__automotive__my_playlist"),
    TStringBuf("onboarding__automotive__rock"),
    TStringBuf("onboarding__automotive__jazz")
};

const TVector<TStringBuf> MUSIC = {
    TStringBuf("onboarding__automotive__imagine_dragons")
};

const TVector<TStringBuf> FM = {
    TStringBuf("onboarding__automotive__europa_plus")
};

const TVector<TStringBuf> SOUND = {
    TStringBuf("onboarding__automotive__volume_up"),
    TStringBuf("onboarding__automotive__volume_down")
};

const TVector<TStringBuf> OTHER = {
    TStringBuf("onboarding__get_weather__tomorrow"),
    TStringBuf("onboarding__navi__currency__dollar")
};

const TVector<TStringBuf> WHAT_CAN_YOU_DO = {
    TStringBuf("onboarding__what_can_you_do")
};

const TVector<TStringBuf> SET = {
    TStringBuf("onboarding__navi__set__accident"),
    TStringBuf("onboarding__navi__set__conversation")
};

const TVector<TStringBuf> ROUTE = {
    TStringBuf("onboarding__navi__find_poi__gasoline"),
    TStringBuf("onboarding__navi__reset_route")
};

const TVector<TStringBuf> SKILL = {
    TStringBuf("onboarding__navi__play__city"),
    TStringBuf("onboarding__navi__play__believe"),
    TStringBuf("onboarding__navi__talk")
};

const TVector<TStringBuf> PLAYER = {
    TStringBuf("onboarding__automotive__next_track"),
    TStringBuf("onboarding__automotive__pause")
};

enum ESuggestGroup {
    ST_NoRoute,
    ST_WithRoute,
    ST_InRadio,
    ST_InMusic,
    ST_InFM,
    ST_InBT,
    ST_InAUX
};

// Suggest group of groups
const TMap<ESuggestGroup, TVector<TVector<TStringBuf>>> AUTOMOTIVE_SUGGESTS = {
    {
        ST_NoRoute, { GO_TO, RADIO, FM, SOUND, WHAT_CAN_YOU_DO, OTHER }
    },
    {
        ST_WithRoute, { SET, ROUTE, SKILL }
    },
    {
        ST_InRadio, { SOUND, PLAYER, RADIO }
    },
    {
        ST_InMusic, { SOUND, PLAYER, RADIO }
    },
    {
        ST_InFM, { FM, SOUND, WHAT_CAN_YOU_DO }
    },
    {
        ST_InBT, { SOUND, PLAYER, RADIO }
    },
    {
        ST_InAUX, { SOUND, PLAYER, RADIO }
    },
};

const int MAX_SUGGESTS = 3;

void AddSuggestGroup(TContext& ctx, ESuggestGroup suggestType) {
    TVector<TStringBuf> suggestCandidates = {};
    const auto& suggestGroups = AUTOMOTIVE_SUGGESTS.find(suggestType);
    if (suggestGroups == AUTOMOTIVE_SUGGESTS.end()) {
        return;
    }
    for (const auto& suggestGroup: suggestGroups->second) {
        suggestCandidates.insert(std::end(suggestCandidates), std::begin(suggestGroup), std::end(suggestGroup));
    }
    if (NMusic::IsYaMusicSupported(ctx)) {
        suggestCandidates.insert(std::end(suggestCandidates), std::begin(MUSIC), std::end(MUSIC));
    }
    Y_ASSERT(suggestCandidates.size() > 1);
    for (int i = 0; i < MAX_SUGGESTS && suggestCandidates.size() > 0; i++) {
        size_t num = ctx.GetRng().RandomInteger(suggestCandidates.size() - 1);
        ctx.AddSuggest(suggestCandidates[num]);
        std::swap(suggestCandidates[num], suggestCandidates[suggestCandidates.size() - 1]);
        suggestCandidates.pop_back();
    }
}

ESuggestGroup ChooseSuggestGroup(TStringBuf screenState, TStringBuf currentSource, bool hasCurrentRoute) {
    if (screenState ==  MAIN_SS || screenState == NAVI_SS) {
        if (hasCurrentRoute) {
            return ESuggestGroup::ST_WithRoute;
        } else {
            return ESuggestGroup::ST_NoRoute;
        }
    } else if (screenState == MUSIC_SS) {
        if (currentSource == YA_MUSIC_S) {
            return ESuggestGroup::ST_InMusic;
        } else if (currentSource == YA_RADIO_S) {
            return ESuggestGroup::ST_InRadio;
        } else if (currentSource == FM_S) {
            return ESuggestGroup::ST_InFM;
        } else if (currentSource == BT_S) {
            return ESuggestGroup::ST_InBT;
        } else if (currentSource == AUX_S) {
            return ESuggestGroup::ST_InAUX;
        }
    }
    // fallback
    return ESuggestGroup::ST_NoRoute;
}

void AddSuggests(TContext& ctx) {
    const auto& deviceState = ctx.Meta().DeviceState();
    TStringBuf screenState;
    TStringBuf currentSource;
    if (deviceState.HasAutomotive() && deviceState.Automotive().HasScreenState()) {
        screenState = deviceState.Automotive().ScreenState();
    }
    if (deviceState.HasAutomotive() && deviceState.Automotive().HasAudio() && deviceState.Automotive().Audio().HasCurrentSource()) {
        currentSource = deviceState.Automotive().Audio().CurrentSource();
    } else if (deviceState.HasUma() && deviceState.Uma().HasActiveMediaSource() && deviceState.Uma().ActiveMediaSource().HasName()) {
        currentSource = deviceState.Uma().ActiveMediaSource().Name();
    }
    AddSuggestGroup(ctx, ChooseSuggestGroup(screenState, currentSource, deviceState.NavigatorState().HasCurrentRoute()));
}

} // NAutomotive

TSessionStartFormHandler::TSessionStartFormHandler()
{
}

TResultValue TSessionStartFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();

    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ctx.FormName() == SessionStartFormName
                                                         ? NAlice::NProductScenarios::COMMANDS_OTHER
                                                         : NAlice::NProductScenarios::ONBOARDING);

    if (ctx.FormName() == SessionStartFormName &&
        ctx.HasExpFlag(EXPERIMENTAL_FLAG_NO_GREETINGS) &&
        !ctx.ClientFeatures().SupportsNavigator())
    {
        return TResultValue();
    }

    if (ctx.MetaClientInfo().IsYaAuto()) {
        if (ctx.Meta().DeviceState().HasAutomotive() || ctx.Meta().DeviceState().HasUma()) {
            NAutomotive::AddSuggests(ctx);
        }
        return TResultValue();
    }

    if (ctx.FormName() == SessionStartFormName &&
        ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_GREETINGS_NOCARDS) &&
        !ctx.ClientFeatures().SupportsNavigator() &&
        !ctx.MetaClientInfo().IsYaMusic() &&
        !ctx.MetaClientInfo().IsSmartSpeaker()) {
        if (NExternalSkill::TSkillRecommendationInitializer::SetAsResponse(ctx, NExternalSkill::EServiceRequestCard::GetGreetings)) {
            return ctx.RunResponseFormHandler();
        }
    }

    const auto& requiredSuggests = GetSuggests(ctx);
    Y_ASSERT(!requiredSuggests.empty());
    size_t num = r.Ctx().GetRng().RandomInteger(requiredSuggests.size());
    if (ctx.FormName() != OnboardingFormName) {
        ctx.AddOnboardingSuggest();
    }
    for (TStringBuf i : requiredSuggests[num]) {
        ctx.AddSuggest(i);
    }
    if (ctx.FormName() != SessionStartFormName || !ctx.ClientFeatures().SupportsOpenLink()) {
        ctx.AddSearchSuggest();
    }
    return TResultValue();
}

void TSessionStartFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TSessionStartFormHandler>();
    };

    handlers->emplace(SessionStartFormName, handler);
    handlers->emplace(OnboardingFormName, handler);
}

TContext::TPtr TSessionStartFormHandler::SetAsResponse(TContext& ctx) {
    return ctx.SetResponseForm(OnboardingFormName, false);
}

TContext::TPtr TSessionStartFormHandler::SetSessionStartAsResponse(TContext& ctx) {
    return ctx.SetResponseForm(SessionStartFormName, false);
}

}
