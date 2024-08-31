#include "onboarding.h"

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/external_skill_recommendation/skill_recommendation.h>
#include <alice/bass/forms/reminders/helpers.h>
#include <alice/bass/forms/session_start.h>
#include <alice/bass/forms/radio.h>
#include <alice/bass/forms/tv/onboarding.h>
#include <alice/bass/forms/video/onboarding.h>
#include <alice/bass/forms/video/web_os_helper.h>
#include <alice/bass/forms/watch/onboarding.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/resource/resource.h>

#include <util/generic/array_size.h>
#include <util/generic/hash.h>
#include <util/random/shuffle.h>

namespace NBASS {

namespace {

constexpr TStringBuf ONBOARDING_CANCEL_FORM_NAME = "personal_assistant.scenarios.onboarding__cancel";
constexpr TStringBuf ONBOARDING_DETAILS_FORM_NAME = "personal_assistant.scenarios.onboarding__details";
constexpr TStringBuf ONBOARDING_NEXT_FORM_NAME = "personal_assistant.scenarios.onboarding__next";

constexpr TStringBuf PODCAST_ONBOARDING_FORM_NAME = "personal_assistant.scenarios.music_podcast_onboarding";
constexpr TStringBuf PODCAST_ONBOARDING_NEXT_FORM_NAME = "personal_assistant.scenarios.music_podcast_onboarding__next";
constexpr TStringBuf RADIO_PLAY_ONBOARDING_FORM_NAME = "personal_assistant.scenarios.radio_play_onboarding";
constexpr TStringBuf RADIO_PLAY_ONBOARDING_NEXT_FORM_NAME = "personal_assistant.scenarios.radio_play_onboarding__next";

constexpr TStringBuf SPECIFIC_ONBOARDING_RESULTS = "specific_onboarding_results";
constexpr TStringBuf ATTENTION_NO_MORE_ITEMS = "onboarding_no_more_items";

constexpr size_t SPECIFIC_ONBOARDING_NUM_ITEMS_PER_PAGE = 3;

// Helper struct for cards initialization.
struct TOnboardingCard {
    TStringBuf Items[5];
    bool (*Predicate)(const NBASS::TContext& ctx) = nullptr;
};

// Helper function for condition negation.
template<bool P(const NBASS::TContext& ctx)>
bool Not(const NBASS::TContext& ctx) {
    return !P(ctx);
}

// Onboarding scenarios sets, random one will be returned on the first request
// and next one in circular order for subsequent requests.
const TVector<TOnboardingCard> OnboardingSet {
    {
        {
            TStringBuf("onboarding__meaning_of_the_name"),
            TStringBuf("onboarding__get_time"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__taxi"),
            TStringBuf("onboarding__sos")
        }
    },
    {
        {
            TStringBuf("onboarding__search"),
            TStringBuf("onboarding__show_route"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__how_much"),
            TStringBuf("onboarding__taxi2")
        }
    },
    {
        {
            TStringBuf("onboarding__find_poi"),
            TStringBuf("onboarding__open_site"),
            TStringBuf("onboarding__music_what_is_playing"),
            TStringBuf("onboarding__weather"),
            TStringBuf("onboarding__believe_or_not_wintergames")
        }
    },
    {
        {
            TStringBuf("onboarding__show_route2"),
            TStringBuf("onboarding__weather"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__open_site2"),
            TStringBuf("onboarding__gc_skill")
        }
    },
    {
        {
            TStringBuf("onboarding__alice_songs"),
            TStringBuf("onboarding__open_application2"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__convert"),
            TStringBuf("onboarding__game_records")
        },
        Not<NBASS::NReminders::AreTimersAndAlarmsEnabled>
    },
    {
        {
            TStringBuf("onboarding__find_poi"),
            TStringBuf("onboarding__convert"),
            TStringBuf("onboarding__music_what_is_playing"),
            TStringBuf("onboarding__market_present"),
            TStringBuf("onboarding__games")
        },
        Not<NBASS::NReminders::AreTimersAndAlarmsEnabled>
    },
    {
        {
            TStringBuf("onboarding__get_date"),
            TStringBuf("onboarding__how_much"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__get_my_location"),
            TStringBuf("onboarding__gc_skill2")
        },
        Not<NBASS::NReminders::AreTimersAndAlarmsEnabled>
    },
    {
        {
            TStringBuf("onboarding__alice_songs"),
            TStringBuf("onboarding__open_application2"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__get_my_location"),
            TStringBuf("onboarding__alarm")
        },
        NBASS::NReminders::AreTimersAndAlarmsEnabled
    },
    {
        {
            TStringBuf("onboarding__find_poi"),
            TStringBuf("onboarding__convert"),
            TStringBuf("onboarding__music_what_is_playing"),
            TStringBuf("onboarding__timer"),
            TStringBuf("onboarding__games")
        },
        NBASS::NReminders::AreTimersAndAlarmsEnabled
    },
    {
        {
            TStringBuf("onboarding__get_date"),
            TStringBuf("onboarding__how_much"),
            TStringBuf("onboarding__music_fairy_tale"),
            TStringBuf("onboarding__get_my_location"),
            TStringBuf("onboarding__gc_skill2")
        },
        NBASS::NReminders::AreTimersAndAlarmsEnabled
    },
};

const THashMap<TStringBuf, TStringBuf> OnboardingIcons = {
    { TStringBuf("onboarding__alarm"), "Alarm" },
    { TStringBuf("onboarding__alice_songs"), "Song" },
    { TStringBuf("onboarding__alice_songs2"), "Song" },
    { TStringBuf("onboarding__believe_or_not_wintergames"), "True_False" },
    { TStringBuf("onboarding__convert"), "Change" },
    { TStringBuf("onboarding__find_poi"), "Map" },
    { TStringBuf("onboarding__heads_or_tails"), "H&T" },
    { TStringBuf("onboarding__heads_or_tails2"), "H&T" },
    { TStringBuf("onboarding__games"), "Game" },
    { TStringBuf("onboarding__game_records"), "Cup" },
    { TStringBuf("onboarding__gc_skill"), "TalktoAlice" },
    { TStringBuf("onboarding__gc_skill2"), "TalktoAlice" },
    { TStringBuf("onboarding__get_date"), "Date" },
    { TStringBuf("onboarding__get_my_location"), "Navigation" },
    { TStringBuf("onboarding__get_time"), "Time" },
    { TStringBuf("onboarding__market_present"), "Gift" },
    { TStringBuf("onboarding__market_present2"), "Gift" },
    { TStringBuf("onboarding__music"), "Music" },
    { TStringBuf("onboarding__music2"), "Music" },
    { TStringBuf("onboarding__open_application"), "Open_App" },
    { TStringBuf("onboarding__open_application2"), "Open_App" },
    { TStringBuf("onboarding__open_site"), "Open_site" },
    { TStringBuf("onboarding__open_site2"), "Open_site" },
    { TStringBuf("onboarding__search"), "Search" },
    { TStringBuf("onboarding__search_factoid"), "Fact" },
    { TStringBuf("onboarding__show_route"), "Route" },
    { TStringBuf("onboarding__show_route2"), "Route" },
    { TStringBuf("onboarding__show_traffic"), "Traffic" },
    { TStringBuf("onboarding__sos"), "SOS" },
    { TStringBuf("onboarding__sos2"), "SOS" },
    { TStringBuf("onboarding__taxi"), "Taxi" },
    { TStringBuf("onboarding__taxi2"), "Taxi" },
    { TStringBuf("onboarding__timer"), "Timer" },
    { TStringBuf("onboarding__toast"), "Toast" },
    { TStringBuf("onboarding__toast2"), "Toast" },
    { TStringBuf("onboarding__weather"), "Wheather" },
    { TStringBuf("onboarding__music_fairy_tale"), "FairyTale" },
    { TStringBuf("onboarding__how_much"), "HowMuch" },
    { TStringBuf("onboarding__music_what_is_playing"), "Shazamilka" },
    { TStringBuf("onboarding__meaning_of_the_name"), "Fact" },
};

const NSc::TValue FILTRED_TOP_PODCASTS = NSc::TValue::FromJsonThrow(NResource::Find("podcasts_top_filtred"));

const TVector<TOnboardingCard>& SelectCardSet(const NBASS::TContext& /*ctx*/) {
    return OnboardingSet;
}

// Returns index of random card with even distribution. Due to predicate which
// removes some cards from the list even distribution requires special handling.
i64 GetRandomCardIndex(NBASS::TContext& ctx) {
    const auto& selectedSet = SelectCardSet(ctx);
    std::vector<i64> variants;
    variants.reserve(selectedSet.size());
    for (size_t i = 0; i < selectedSet.size(); i++) {
        if (!selectedSet[i].Predicate || selectedSet[i].Predicate(ctx)) {
            variants.push_back(i);
        }
    }
    if (variants.empty()) {
        return -1;
    }
    return variants[ctx.GetRng().RandomInteger(variants.size())];
}

using TFetchItemsCallback = std::function<TResultValue(TContext& ctx, NSc::TValue& result)>;

TResultValue HandleSpecificOnboarding(TContext& ctx, TFetchItemsCallback fetchItems) {
    TContext::TSlot* allItemsSlot = ctx.GetOrCreateSlot("all_items", SPECIFIC_ONBOARDING_RESULTS);
    if (IsSlotEmpty(allItemsSlot)) {
        if(auto error = fetchItems(ctx, allItemsSlot->Value)) {
            return error;
        }
    }

    const NSc::TArray& allItems = allItemsSlot->Value.GetArray();

    TContext::TSlot* pageIdxSlot = ctx.GetOrCreateSlot("page_index", "num");
    size_t pageIdx = pageIdxSlot->Value.GetIntNumber(0);
    size_t pageStartIdx = pageIdx * SPECIFIC_ONBOARDING_NUM_ITEMS_PER_PAGE;
    size_t pageSize = Min(SPECIFIC_ONBOARDING_NUM_ITEMS_PER_PAGE, allItems.size() - pageStartIdx);

    NSc::TValue pageItems;
    if (allItems.size() > 0 && pageStartIdx >= allItems.size()) {
        ctx.AddAttention(ATTENTION_NO_MORE_ITEMS);
    } else if (pageStartIdx < allItems.size()) {
        for (size_t i = 0; i < pageSize; ++i) {
            pageItems.Push(allItems[pageStartIdx + i]);
        }
    }

    pageIdxSlot->Value.SetIntNumber(pageIdx + 1);
    ctx.CreateSlot("page_items", SPECIFIC_ONBOARDING_RESULTS, true /* optional */, pageItems);

    return TResultValue();
}

template<typename Items>
NSc::TValue ExtractTitles(const Items& items) {
    NSc::TValue result;
    for (const auto& item : items) {
        NSc::TValue resultItem;
        resultItem["title"] = item["title"];
        result.Push(resultItem);
    }

    return result;
}

TResultValue FetchRadioStations(TContext& ctx, NSc::TValue& resultsValue) {
    TVector<NSc::TValue> radioList;
    if (auto err = NRadio::FetchOrderedRadioList(ctx, radioList)) {
        return err;
    }

    resultsValue = ExtractTitles(radioList);

    return TResultValue();
}

TResultValue FetchPodcasts(TContext& ctx, NSc::TValue& resultsValue) {
    resultsValue = ExtractTitles(FILTRED_TOP_PODCASTS.GetArray());

    auto& items = resultsValue.GetArrayMutable();
    Shuffle(items.begin(), items.end(), ctx.GetRandGeneratorInitializedWithEpoch());

    return TResultValue();
}

} // namespace

// static
const TStringBuf TOnboardingHandler::OnboardingFormName = TStringBuf("personal_assistant.scenarios.onboarding");

TResultValue TOnboardingHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);

    if (ctx.MetaClientInfo().IsLegatus()) {
        NVideo::AddWebOSLaunchAppCommandForOnboarding(ctx);
        return TResultValue();
    }

    TStringBuf formName = ctx.FormName();
    if (formName == RADIO_PLAY_ONBOARDING_FORM_NAME || formName == RADIO_PLAY_ONBOARDING_NEXT_FORM_NAME) {
        return HandleSpecificOnboarding(ctx, FetchRadioStations);
    }
    if (formName == PODCAST_ONBOARDING_FORM_NAME || formName == PODCAST_ONBOARDING_NEXT_FORM_NAME) {
        return HandleSpecificOnboarding(ctx, FetchPodcasts);
    }

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        return NVideo::StartOnboarding(ctx);
    }

    if (ctx.MetaClientInfo().IsElariWatch()) {
        return NWatch::StartOnboarding(ctx, TStringBuf("main"));
    }

    if (ctx.MetaClientInfo().IsTvDevice()) {
        return NTvCommon::StartOnboarding(ctx, TStringBuf("main"));
    }

    const TSlot* const slot = ctx.GetSlot("mode");
    const bool isGreetings = !IsSlotEmpty(slot) && slot->Value.GetString() == "get_greetings";
    const bool isFirstSession = !IsSlotEmpty(slot) && slot->Value.GetString() == "onboarding";
    const bool isOnboarding = !isGreetings && !isFirstSession;
    NExternalSkill::EServiceRequestCard djCard = NExternalSkill::EServiceRequestCard::Onboarding;
    if (isGreetings) {
        djCard = NExternalSkill::EServiceRequestCard::GetGreetings;
    } else if (isFirstSession) {
        djCard = NExternalSkill::EServiceRequestCard::FirstSessionOnboarding;
    }

    if (!ctx.ClientFeatures().SupportsDivCards()) {
        bool useDjService = false;
        if (ctx.MetaClientInfo().IsNavigator()) {
            useDjService = (isOnboarding && ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_NAVI)) ||
                            isGreetings;
        } else if (ctx.MetaClientInfo().IsYaMusic()) {
            //There's no greetings in Ya.Music yet so we just go to service no matter the mode
            useDjService = ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_YAMUSIC);
        } else if (!ctx.MetaClientInfo().IsYaAuto() && !ctx.MetaClientInfo().IsSmartSpeaker()) {
            useDjService = (isOnboarding && ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_NOCARDS)) ||
                            (isGreetings && ctx.HasExpFlag(EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_GREETINGS_NOCARDS));
        }
        if (useDjService) {
            if (NExternalSkill::TSkillRecommendationInitializer::SetAsResponse(ctx, djCard)) {
                return ctx.RunResponseFormHandler();
            }
        }
        if (TSessionStartFormHandler::SetAsResponse(ctx)) {
            return ctx.RunResponseFormHandler(); // Fallback to old onboarding form
        }
        return TError(TError::EType::ONBOARDINGERROR, TStringBuf("Non-card onboarding_failed"));
    }

    //For DIV-card supporting clients
    if (NExternalSkill::TSkillRecommendationInitializer::SetAsResponse(ctx, djCard)) {
        return ctx.RunResponseFormHandler();
    }

    ctx.AddTextCardBlock("onboarding__set_message");
    const auto& selectedSet = SelectCardSet(ctx);

    TContext::TSlot* num = ctx.GetOrCreateSlot(TStringBuf("set_number"), TStringBuf("num"));
    i64 index = -1;
    if (num->Value.IsNull()) {
        index = GetRandomCardIndex(ctx);
    } else {
        index = (num->Value.GetIntNumber() + 1) % selectedSet.size();

        size_t count = 0;
        while (selectedSet[index].Predicate && !selectedSet[index].Predicate(ctx)) {
            // This card is not enabled for given client, get next one.
            index = (index + 1) % selectedSet.size();
            if (++count >= selectedSet.size()) {
                index = -1;
                break;
            }
        }
    }
    if (index < 0) {
        LOG(ERR) << "No onboarding cards enabled for given client!" << Endl;
        return TError(TError::EType::SYSTEM, TStringBuf("no_onboarding_cards"));
    }
    Y_ASSERT(static_cast<size_t>(index) < selectedSet.size());
    num->Value.SetIntNumber(index);

    NSc::TValue cases;
    NSc::TValue icons;
    for (const auto& i : selectedSet[index].Items) {
        cases.Push(i);

        const TStringBuf* iconType = OnboardingIcons.FindPtr(i);
        if (!iconType) {
            LOG(ERR) << "No icon for onboarding case " << i << Endl;
            // In case of error use search icon as fallback.
            iconType = OnboardingIcons.FindPtr(TStringBuf("onboarding__search"));
        }
        if (iconType) {
            const TAvatar* avatar = ctx.Avatar(TStringBuf("onboard"), *iconType);
            if (avatar) {
                icons.Push().SetString(avatar->Https);
            } else {
                LOG(ERR) << "Missing icon file " << *iconType << Endl;
                icons.Push().SetNull();
            }
        } else {
            icons.Push().SetNull();
        }
    }
    NSc::TValue data;
    data["cases"] = cases;
    if (icons.ArraySize()) {
        data["icons"] = icons;
    }
    data["footer_url"].SetString("https://dialogs.yandex.ru/store/?utm_source=alice&utm_medium=feature");
    ctx.AddDivCardBlock("onboarding", std::move(data));

    NSc::TValue formUpdate;
    formUpdate["name"] = OnboardingFormName;
    formUpdate["resubmit"].SetBool(true);
    NSc::TValue numValue;
    num->ToJson(&numValue, nullptr);
    formUpdate["slots"].SetArray().Push(numValue);
    ctx.AddSuggest("onboarding__next", NSc::Null(), formUpdate);

    // If there is no utterance i.e. form as activated on the very first use,
    // don't add search fallback because it will have empty query.
    if (!ctx.Meta().Utterance().Get().empty()) {
        ctx.AddSearchSuggest();
    }
    return TResultValue();
}

TResultValue TOnboardingCancelHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);
    return TResultValue();
}

TResultValue TOnboardingNextHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);
    if (r.Ctx().MetaClientInfo().IsSmartSpeaker()) {
        return NVideo::ContinueOnboarding(r.Ctx());
    }
    if (r.Ctx().MetaClientInfo().IsElariWatch()) {
        return NWatch::ContinueOnboarding(r.Ctx());
    }
    if (r.Ctx().MetaClientInfo().IsTvDevice()) {
        return NTvCommon::ContinueOnboarding(r.Ctx());
    }
    return TResultValue();
}

// static
void TOnboardingHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TOnboardingHandler>();
    };

    handlers->emplace(OnboardingFormName, handler);
    handlers->emplace(ONBOARDING_DETAILS_FORM_NAME, handler);
    handlers->emplace(PODCAST_ONBOARDING_FORM_NAME, handler);
    handlers->emplace(PODCAST_ONBOARDING_NEXT_FORM_NAME, handler);
    handlers->emplace(RADIO_PLAY_ONBOARDING_FORM_NAME, handler);
    handlers->emplace(RADIO_PLAY_ONBOARDING_NEXT_FORM_NAME, handler);
}

// static
TContext::TPtr TOnboardingHandler::SetAsResponse(TContext& ctx) {
    return ctx.SetResponseForm(OnboardingFormName, false);
}

// static
void TOnboardingCancelHandler::Register(THandlersMap* handlers) {
    handlers->emplace(ONBOARDING_CANCEL_FORM_NAME, [] () { return MakeHolder<TOnboardingCancelHandler>(); });
}

// static
void TOnboardingNextHandler::Register(THandlersMap* handlers) {
    handlers->emplace(ONBOARDING_NEXT_FORM_NAME, [] () { return MakeHolder<TOnboardingNextHandler>(); });
}

} // namespace NBASS
