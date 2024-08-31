#include "games_onboarding.h"

#include "onboarding.h"
#include "watch/onboarding.h"

#include "external_skill_recommendation/skill_recommendation.h"

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/array_size.h>
#include <util/generic/hash.h>


namespace {

// Games set.
const TStringBuf GamesSets[][5] = {
    {
        TStringBuf("games_onboarding__quest"),
        TStringBuf("games_onboarding__this_day_in_history"),
        TStringBuf("games_onboarding__zoology"),
        TStringBuf("games_onboarding__cities"),
        TStringBuf("games_onboarding__what_comes_first")
    },
    {
        TStringBuf("games_onboarding__cities"),
        TStringBuf("games_onboarding__magic_ball"),
        TStringBuf("games_onboarding__words_in_word"),
        TStringBuf("games_onboarding__believe_or_not"),
        TStringBuf("games_onboarding__records")
    },
    {
        TStringBuf("games_onboarding__words_in_word"),
        TStringBuf("games_onboarding__guess_actor"),
        TStringBuf("games_onboarding__find_extra"),
        TStringBuf("games_onboarding__guess_the_song"),
        TStringBuf("games_onboarding__lao_wai")
    },
    {
        TStringBuf("games_onboarding__guess_the_song"),
        TStringBuf("games_onboarding__divination"),
        TStringBuf("games_onboarding__quest"),
        TStringBuf("games_onboarding__believe_or_not"),
        TStringBuf("games_onboarding__records")
    }
};

// Suggests.
const TStringBuf GamesSuggests[][7] = {
    {
        TStringBuf("games_onboarding__quest"),
        TStringBuf("games_onboarding__this_day_in_history"),
        TStringBuf("games_onboarding__zoology"),
        TStringBuf("games_onboarding__what_comes_first"),
        TStringBuf("games_onboarding__guess_actor"),
        TStringBuf("games_onboarding__guess_the_song"),
        TStringBuf("games_onboarding__divination")
    },
    {
        TStringBuf("games_onboarding__magic_ball"),
        TStringBuf("games_onboarding__words_in_word"),
        TStringBuf("games_onboarding__believe_or_not"),
        TStringBuf("games_onboarding__find_extra"),
        TStringBuf("games_onboarding__guess_actor"),
        TStringBuf("games_onboarding__guess_the_song"),
        TStringBuf("games_onboarding__divination")
    },
    {
        TStringBuf("games_onboarding__cities"),
        TStringBuf("games_onboarding__believe_or_not"),
        TStringBuf("games_onboarding__quest"),
        TStringBuf("games_onboarding__this_day_in_history"),
        TStringBuf("games_onboarding__zoology"),
        TStringBuf("games_onboarding__what_comes_first"),
        TStringBuf("games_onboarding__divination")
    },
    {
        TStringBuf("games_onboarding__cities"),
        TStringBuf("games_onboarding__magic_ball"),
        TStringBuf("games_onboarding__words_in_word"),
        TStringBuf("games_onboarding__find_extra"),
        TStringBuf("games_onboarding__this_day_in_history"),
        TStringBuf("games_onboarding__what_comes_first"),
        TStringBuf("games_onboarding__guess_actor")
    },
};

// Map game code value to icon name without size and extension, see https://proxy.sandbox.yandex-team.ru/397280234
const THashMap<TStringBuf, TStringBuf> GamesIcons = {
    { TStringBuf("games_onboarding__antistress"), TStringBuf("Antistress") },
    { TStringBuf("games_onboarding__believe_or_not"), TStringBuf("True_False") },
    { TStringBuf("games_onboarding__cities"), TStringBuf("Cities") },
    { TStringBuf("games_onboarding__divination"), TStringBuf("Dvntn_by_book") },
    { TStringBuf("games_onboarding__find_extra"), TStringBuf("Select_extra") },
    { TStringBuf("games_onboarding__guess_actor"), TStringBuf("Guess_actor") },
    { TStringBuf("games_onboarding__guess_the_song"), TStringBuf("Guess_melody") },
    { TStringBuf("games_onboarding__lao_wai"), TStringBuf("LaoWai") },
    { TStringBuf("games_onboarding__magic_ball"), TStringBuf("Magic_ball") },
    { TStringBuf("games_onboarding__records"), TStringBuf("Cup") },
    { TStringBuf("games_onboarding__quest"), TStringBuf("Quest") },
    { TStringBuf("games_onboarding__this_day_in_history"), TStringBuf("History_day") },
    { TStringBuf("games_onboarding__what_comes_first"), TStringBuf("Before_after") },
    { TStringBuf("games_onboarding__words_in_word"), TStringBuf("Word") },
    { TStringBuf("games_onboarding__zoology"), TStringBuf("Zoo") },
    { TStringBuf("games_onboarding__disney_monsters"), TStringBuf("Frnkst") },
};

}

namespace NBASS {

TResultValue TGamesOnboardingHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);
    if (ctx.MetaClientInfo().IsElariWatch()) {
        TContext::TPtr onboardingForm = TOnboardingHandler::SetAsResponse(ctx);
        if (onboardingForm) {
            return NWatch::StartOnboarding(*onboardingForm, TStringBuf("games"));
        }
    }

    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_GAMES_ONBOARDING) || ctx.ClientFeatures().SupportsDivCards()) {
        if (NExternalSkill::TSkillRecommendationInitializer::SetAsResponse(ctx, NExternalSkill::EServiceRequestCard::GamesOnboarding)) {
            return ctx.RunResponseFormHandler();
        }
        Y_STATS_INC_COUNTER("bass_skill_recommendation_service_error");
        return TError{TError::EType::SKILLSERVICEERROR, "Games onboarding: Skill recommendation service failure"};
    }

    TContext::TSlot* num = ctx.GetOrCreateSlot(TStringBuf("set_number"), TStringBuf("num"));
    if (num->Value.IsNull()) {
        num->Value.SetIntNumber(ctx.GetRng().RandomInteger(Y_ARRAY_SIZE(GamesSets)));
    } else {
        num->Value.SetIntNumber((num->Value.GetIntNumber() + 1) % Y_ARRAY_SIZE(GamesSets));
    }
    i64 index = num->Value.GetIntNumber();

    // Client doesn't support div card, use text only version and just choose random game.
    ctx.AddTextCardBlock(GamesSets[index][ctx.GetRng().RandomInteger(Y_ARRAY_SIZE(GamesSets[index]))]);

    for (const auto& i : GamesSuggests[index]) {
        ctx.AddSuggest(i);
    }

    if (!ctx.Meta().Utterance().Get().empty()) {
        ctx.AddSearchSuggest();
    }

    return TResultValue();
}

// static
const TStringBuf TGamesOnboardingHandler::GameOnboardingFormName = TStringBuf("personal_assistant.scenarios.games_onboarding");

// static
void TGamesOnboardingHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TGamesOnboardingHandler>();
    };

    handlers->emplace(GameOnboardingFormName, handler);
}

}
