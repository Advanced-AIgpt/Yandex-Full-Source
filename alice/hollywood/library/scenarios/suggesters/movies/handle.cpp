#include "handle.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/suggesters/common/utils.h>
#include <alice/hollywood/library/scenarios/suggesters/movies/proto/movie_suggest_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>
#include <alice/library/video_common/defs.h>
#include <alice/protos/data/video/video.pb.h>

#include <util/generic/hash_set.h>
#include <util/string/cast.h>

#include <memory>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf EXP_FILTER_BY_GENRE = "hw_movie_suggest_filter_by_genre";
constexpr TStringBuf MAX_PERSUASION_COUNT_EXP_PREFIX = "max_persuasion_count=";
constexpr ui32 DEFAULT_MAX_PERSUASION_COUNT = 0;

const TString FRAME = "alice.movie_suggest";

const TString SHOW_ALL_INTENT = ".show_all";
const TString SHOW_CARTOONS_INTENT = ".show_cartoons";
const TString SHOW_MOVIES_INTENT = ".show_movies";

const TString NLG_TEMPLATE = "movie_suggest";

constexpr TStringBuf SHOW_DESCRIPTION_TAG = "video_show_description";

const TString ALREADY_WATCHED_FRAME_NAME = "alice.movie_suggest.already_watched";
const TString CONFIRM_FRAME_NAME = "alice.general_conversation.proactivity_agree";
const TString DECLINE_FRAME_NAME = "alice.movie_suggest.decline";

const TString SLOT_ALREADY_WATCHED = "already_watched";

const THashSet<TString> COPY_SLOTS = {
    TString{NVideoCommon::SLOT_CONTENT_TYPE},
    TString{NVideoCommon::SLOT_GENRE}
};

using TMovieSuggestBuilder = TSuggestResponseBuilder<TMovieSuggestState>;

ui32 GetMaxPersuasionCount(const THashMap<TString, TMaybe<TString>>& expFlags) {
    ui32 maxPersuasionCount = DEFAULT_MAX_PERSUASION_COUNT;
    for (const auto& [flag, maybeValue] : expFlags) {
        if (const auto maybeFlagValue = TryGetFlagValue(flag, MAX_PERSUASION_COUNT_EXP_PREFIX)) {
            if (TryFromString(*maybeFlagValue, maxPersuasionCount)) {
                break;
            }
        }
    }

    return maxPersuasionCount;
}

bool IsTvPluggedIn(const TScenarioRunRequestWrapper& request) {
    return request.ClientInfo().IsSmartSpeaker() && request.BaseRequestProto().GetInterfaces().GetIsTvPlugged();
}

TMovieRecommender::TRestrictions GetRestrictions(const TScenarioRunRequestWrapper& request,
                                                 const TMovieSuggestBuilder& suggestBuilder)
{
    TMovieRecommender::TRestrictions restrictions;
    restrictions.Age = GetContentRestrictionLevel(request.ContentRestrictionLevel());
    restrictions.ContentType = suggestBuilder.TryGetSlotValue(NVideoCommon::SLOT_CONTENT_TYPE);

    const auto genre = suggestBuilder.TryGetSlotValue(NVideoCommon::SLOT_GENRE);
    if (!restrictions.ContentType) {
        restrictions.ContentType = GetContentTypeByGenre(genre);
    }

    if (request.HasExpFlag(EXP_FILTER_BY_GENRE)) {
        restrictions.Genre = genre;
    }

    const auto& suggestedItemIds = suggestBuilder.GetState().GetSuggestionsHistory();
    restrictions.ItemIds.insert(suggestedItemIds.begin(), suggestedItemIds.end());

    return restrictions;
}

const TMovieRecommender::TItem* Recommend(const TMovieRecommender& recommender,
                                          const TMovieRecommender::TRestrictions& restrictions,
                                          const TMaybe<TString>& persuadeAboutItemId,
                                          IRng& rng)
{
    if (persuadeAboutItemId) {
        if (const auto* itemPtr = recommender.GetItemById(*persuadeAboutItemId)) {
            return itemPtr;
        }
    }

    return recommender.Recommend(restrictions, rng);
}

TSemanticFrame BuildConfirmFrameEffect() {
    TSemanticFrame frame;

    frame.SetName(TString{NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO});

    TSemanticFrame::TSlot& slotAction = *frame.AddSlots();
    slotAction.SetName(TString{NVideoCommon::SLOT_ACTION});
    slotAction.SetType(TString{NVideoCommon::SLOT_SELECTION_ACTION_TYPE});
    slotAction.SetValue(ToString(NVideoCommon::ESelectionAction::Play));

    return frame;
}

void AddAlreadyWatchedAction(const TSemanticFrame& semanticFrame, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TFrameAction alreadyWatchedAction;

    alreadyWatchedAction.MutableNluHint()->SetFrameName(ALREADY_WATCHED_FRAME_NAME);

    auto frame = InitCallbackFrameEffect(semanticFrame, TString{FRAME}, COPY_SLOTS);
    frame.AddSlots()->SetName(SLOT_ALREADY_WATCHED);
    *alreadyWatchedAction.MutableCallback() = ToCallback(frame);

    bodyBuilder.AddAction(SLOT_ALREADY_WATCHED, std::move(alreadyWatchedAction));
}

void AddDescriptionDirective(const TMovieRecommender::TItem& item, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TDirective directive;
    auto& descriptionDirective = *directive.MutableShowVideoDescriptionDirective();

    descriptionDirective.SetName(TString{SHOW_DESCRIPTION_TAG});
    *descriptionDirective.MutableItem() = item.VideoItem;

    bodyBuilder.AddDirective(std::move(directive));
}

void FillAnalyticsInfo(const TMovieRecommender::TRestrictions& restrictions,
                       NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder)
{
    if (!restrictions.ContentType) {
        analyticsInfoBuilder.SetIntentName(FRAME + SHOW_ALL_INTENT);
    } else if (restrictions.ContentType == ToString(NVideoCommon::EContentType::Cartoon)) {
        analyticsInfoBuilder.SetIntentName(FRAME + SHOW_CARTOONS_INTENT);
    } else if (restrictions.ContentType == ToString(NVideoCommon::EContentType::Movie)) {
        analyticsInfoBuilder.SetIntentName(FRAME + SHOW_MOVIES_INTENT);
    }
}

} // namespace

TBaseSuggestResponseBuilder::TConfig BuildMovieSuggestConfig() {
    TBaseSuggestResponseBuilder::TConfig config;

    config.AcceptedFrameNames = {FRAME};
    config.NlgTemplate = NLG_TEMPLATE;

    config.ConfirmGranetName = CONFIRM_FRAME_NAME;
    config.DeclineGranetName = DECLINE_FRAME_NAME;

    config.DeclineEffectFrameName = FRAME;
    config.DeclineEffectFrameCopiedSlots = COPY_SLOTS;

    return config;
}

template <>
std::unique_ptr<NScenarios::TScenarioRunResponse> BuildResponse<TMovieRecommender>(
    const TScenarioRunRequestWrapper& request,
    const TMovieRecommender& recommender,
    const TBaseSuggestResponseBuilder::TConfig& config,
    IRng& rng, TRTLogger& logger,
    TNlgWrapper& nlgWrapper)
{
    TRunResponseBuilder builder(&nlgWrapper);
    TMovieSuggestBuilder suggestBuilder(logger, request, config, builder);

    if (!IsTvPluggedIn(request)) {
        LOG_INFO(logger) << "Tv is not plugged in, irrelevant";
        return std::move(suggestBuilder).BuildIrrelevantResponse();
    }

    if (!suggestBuilder.GetFrame()) {
        LOG_INFO(logger) << "Frame was not found, irrelevant";
        return std::move(suggestBuilder).BuildIrrelevantResponse();
    }

    const auto persuadeAboutItemId = suggestBuilder.TryGetSlotValue(SLOT_PERSUADE);
    const auto restrictions = GetRestrictions(request, suggestBuilder);
    const auto* recommendedItem = Recommend(recommender, restrictions, persuadeAboutItemId, rng);
    if (!recommendedItem) {
        return std::move(suggestBuilder).BuildNoMoreRecommendationsResponse();
    }

    auto& bodyBuilder = suggestBuilder.GetBodyBuilder();
    const bool isPersuasionStep = recommendedItem->ItemId == persuadeAboutItemId && recommendedItem->PersuadingText;

    suggestBuilder.AddNlg(recommendedItem->GetText(isPersuasionStep, rng));

    AddAlreadyWatchedAction(suggestBuilder.GetFrame()->ToProto(), bodyBuilder);

    TMaybe<TString> persuadeOnDeclineItemId;
    const bool canPersuade = suggestBuilder.GetState().GetPersuasionCount() < GetMaxPersuasionCount(request.ExpFlags());
    if (recommendedItem->PersuadingText && !isPersuasionStep && canPersuade) {
        persuadeOnDeclineItemId = recommendedItem->ItemId;
    }
    suggestBuilder.AddDeclineAction(persuadeOnDeclineItemId);
    suggestBuilder.AddConfirmAction(BuildConfirmFrameEffect());

    auto& state = suggestBuilder.GetState();
    if (isPersuasionStep) {
        state.SetPersuasionCount(state.GetPersuasionCount() + 1);
    } else {
        state.AddSuggestionsHistory(recommendedItem->ItemId);
        AddDescriptionDirective(*recommendedItem, bodyBuilder);
    }

    FillAnalyticsInfo(restrictions, bodyBuilder.CreateAnalyticsInfoBuilder());

    return std::move(suggestBuilder).BuildSuccessResponse();
}

TMovieSuggestRunHandle::TMovieSuggestRunHandle()
    : Config(BuildMovieSuggestConfig())
{
}

TBaseSuggestResponseBuilder::TConfig TMovieSuggestRunHandle::BuildConfig(const TScenarioRunRequestWrapper&) const {
    return Config;
}

REGISTER_SCENARIO("movie_suggest",
                  AddHandle<TMovieSuggestRunHandle>()
                  .SetResources<TMovieRecommender>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSuggesters::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
