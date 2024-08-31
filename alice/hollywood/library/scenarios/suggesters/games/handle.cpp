#include "handle.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/suggesters/common/utils.h>
#include <alice/hollywood/library/scenarios/suggesters/games/proto/game_suggest_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <util/generic/hash_set.h>

namespace NAlice::NHollywood {

namespace {

const TString FRAME = "alice.game_suggest";
const TString ONBOARDING_FRAME = "alice.external_skill_games_onboarding";
const TString NLG_TEMPLATE = "game_suggest";

const TString CONFIRM_BUTTON_TEXT = "Включи игру";
const TString CONFIRM_FRAME_NAME = "alice.game_suggest.confirm";

const TString DECLINE_BUTTON_TEXT = "Покажи другую";
const TString DECLINE_FRAME_NAME = "alice.game_suggest.decline";

const TString EXTERNAL_SKIL_FRAME_NAME = "alice.external_skill_fixed_activate";
const TString EXTERNAL_SKIL_SLOT_NAME = "fixed_skill_id";
const TString EXTERNAL_SKIL_SLOT_TYPE = "ActivationPhraseExternalSkillId";

constexpr TStringBuf EXP_USE_ONBOARDING_FRAME = "hw_game_suggest_use_onboarding_frame";

using TGameSuggestBuilder = TSuggestResponseBuilder<TGameSuggestState>;

const TGameRecommender::TItem* Recommend(const TGameSuggestState& state,
                                         const TGameRecommender& recommender, IRng& rng)
{
    TGameRecommender::TRestrictions restrictions;
    const auto& suggestedItemIds = state.GetSuggestionsHistory();
    restrictions.ItemIds.insert(suggestedItemIds.begin(), suggestedItemIds.end());

    return recommender.Recommend(restrictions, rng);
}

TSemanticFrame BuildConfirmFrameEffect(const TGameRecommender::TItem& item) {
    TSemanticFrame frame;
    frame.SetName(EXTERNAL_SKIL_FRAME_NAME);

    TSemanticFrame::TSlot& slotAction = *frame.AddSlots();
    slotAction.SetName(EXTERNAL_SKIL_SLOT_NAME);
    slotAction.SetType(EXTERNAL_SKIL_SLOT_TYPE);
    slotAction.SetValue(item.ItemId);

    return frame;
}

} // namespace

TBaseSuggestResponseBuilder::TConfig BuildGamesSuggestConfig(bool useOnboardingFrame) {
    TBaseSuggestResponseBuilder::TConfig config;

    if (useOnboardingFrame) {
        config.AcceptedFrameNames = {FRAME, ONBOARDING_FRAME};
    } else {
        config.AcceptedFrameNames = {FRAME};
    }

    config.NlgTemplate = NLG_TEMPLATE;

    config.ConfirmGranetName = CONFIRM_FRAME_NAME;
    config.ConfirmButtonTitle = CONFIRM_BUTTON_TEXT;

    config.DeclineGranetName = DECLINE_FRAME_NAME;
    config.DeclineButtonTitle = DECLINE_BUTTON_TEXT;

    config.DeclineEffectFrameName = FRAME;

    return config;
}

template <>
std::unique_ptr<NScenarios::TScenarioRunResponse> BuildResponse<TGameRecommender>(
    const TScenarioRunRequestWrapper& request,
    const TGameRecommender& recommender,
    const TBaseSuggestResponseBuilder::TConfig& config,
    IRng& rng, TRTLogger& logger,
    TNlgWrapper& nlgWrapper)
{
    TRunResponseBuilder builder(&nlgWrapper);
    TGameSuggestBuilder suggestBuilder(logger, request, config, builder);

    if (!suggestBuilder.GetFrame()) {
        LOG_INFO(logger) << "Frame was not found, irrelevant";
        return std::move(suggestBuilder).BuildIrrelevantResponse();
    }

    auto& state = suggestBuilder.GetState();

    const auto* recommendedItem = Recommend(state, recommender, rng);
    if (!recommendedItem) {
        return std::move(suggestBuilder).BuildNoMoreRecommendationsResponse();
    }

    suggestBuilder.AddNlg(recommendedItem->GetResponse(rng).Text);
    suggestBuilder.AddDeclineAction(/* persuadeAboutItemId= */ Nothing());
    suggestBuilder.AddConfirmAction(BuildConfirmFrameEffect(*recommendedItem));

    state.AddSuggestionsHistory(recommendedItem->ItemId);

    return std::move(suggestBuilder).BuildSuccessResponse();
}

TBaseSuggestResponseBuilder::TConfig TGameSuggestRunHandle::BuildConfig(
    const TScenarioRunRequestWrapper& request) const
{
    return BuildGamesSuggestConfig(request.HasExpFlag(EXP_USE_ONBOARDING_FRAME));
}

REGISTER_SCENARIO("game_suggest",
                  AddHandle<TGameSuggestRunHandle>()
                  .SetResources<TGameRecommender>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSuggesters::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
