#include "proactivity_candidates.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame/frame.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

#include <util/generic/algorithm.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

constexpr TStringBuf NLG_RENDER_PROACTIVITY_TEXT = "render_proactivity_text";
constexpr TStringBuf NLG_RENDER_PROACTIVITY_SUGGEST = "render_proactivity_suggest";

struct TActionInfo {
    std::function<bool(const TScenarioRunRequestWrapper&, bool)> IsAllowed;
    TStringBuf FrameActionName;
};

bool IsGameSuggestAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool modalModeEnabled) {
    return !modalModeEnabled && requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_SUGGEST_GAME);
}

bool IsMovieAkinatorAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool /*modalModeEnabled*/) {
    if (!requestWrapper.ClientInfo().IsSearchApp()) {
        return false;
    }

    return requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_MOVIE_AKINATOR);
}

bool IsMovieDiscussAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool modalModeEnabled) {
    if (IsMovieDisscussionAllowedByDefault(requestWrapper, modalModeEnabled)) {
        return true;
    }

    return requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_MOVIE_DISCUSS);
}

bool IsMusicDiscussAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool /*modalModeEnabled*/) {
    return requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_MUSIC_DISCUSS);
}

bool IsGameDiscussAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool /*modalModeEnabled*/) {
    return requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_GAME_DISCUSS);
}

bool IsMovieDiscussSpecificAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool modalModeEnabled) {
    if (IsMovieDisscussionAllowedByDefault(requestWrapper, modalModeEnabled)) {
        return true;
    }

    return requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC);
}

bool IsMovieSuggestAllowed(const TScenarioRunRequestWrapper& requestWrapper, bool modalModeEnabled) {
    if (!requestWrapper.BaseRequestProto().GetDeviceState().GetIsTvPluggedIn()) {
        return false;
    }

    if (requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_CHECK_YANDEX_PLUS)) {
        auto userInfo = GetUserInfoProto(requestWrapper);
        if (!userInfo) {
            return false;
        }

        if (!userInfo->GetHasYandexPlus()) {
            return false;
        }
    }

    return !modalModeEnabled && requestWrapper.HasExpFlag(EXP_HW_GCP_PROACTIVITY_SUGGEST_MOVIE);
}

const THashMap<TStringBuf, TVector<TActionInfo>> FRAME_TO_ACTION_MAP = {
    {
        FRAME_PROACTIVITY_ALICE_DO,
        {
            {IsMovieAkinatorAllowed, FRAME_MOVIE_AKINATOR},
            {IsMovieDiscussSpecificAllowed, FRAME_MOVIE_DISCUSS_SPECIFIC},
            {IsMovieSuggestAllowed, FRAME_MOVIE_SUGGEST},
            {IsGameSuggestAllowed, FRAME_GAME_SUGGEST}
        }
    },
    {
        FRAME_PROACTIVITY_BORED,
        {
            {IsMovieAkinatorAllowed, FRAME_MOVIE_AKINATOR},
            {IsMovieDiscussAllowed, FRAME_MOVIE_DISCUSS},
            {IsMusicDiscussAllowed, FRAME_MUSIC_DISCUSS},
            {IsGameDiscussAllowed, FRAME_GAME_DISCUSS},
            {IsMovieDiscussSpecificAllowed, FRAME_MOVIE_DISCUSS_SPECIFIC},
            {IsMovieSuggestAllowed, FRAME_MOVIE_SUGGEST},
            {IsGameSuggestAllowed, FRAME_GAME_SUGGEST}
        }
    }
};

} // namespace

TString GetProactivityIntentFromSearchAction(const TScenarioRunRequestWrapper& requestWrapper, const TString& action, bool modalModeEnabled, IRng& rng) {
    if (action == FRAME_MOVIE_DISCUSS) {
        TVector<TString> availableActions;
        if (IsMovieDisscussionAllowedByDefault(requestWrapper, modalModeEnabled) || requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS)) {
            availableActions.push_back(ToString(FRAME_MOVIE_DISCUSS));
        }
        if (IsMovieDisscussionAllowedByDefault(requestWrapper, modalModeEnabled) || requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC)) {
            availableActions.push_back(ToString(FRAME_MOVIE_DISCUSS_SPECIFIC));
        }
        if (requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MUSIC_DISCUSS)) {
            availableActions.push_back(ToString(FRAME_MUSIC_DISCUSS));
        }
        if (requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_GAME_DISCUSS)) {
            availableActions.push_back(ToString(FRAME_GAME_DISCUSS));
        }
        if (requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_AKINATOR)) {
            availableActions.push_back(ToString(FRAME_MOVIE_AKINATOR));
        }
        if (!availableActions.empty()) {
            if (const auto forcedProactivity = GetExperimentTypedValue<TString>(requestWrapper.ExpFlags(), EXP_HW_GC_PROACTIVITY_FORCE_SOFT)) {
                for (const auto& availableAction : availableActions) {
                    if (forcedProactivity.GetRef() == availableAction) {
                        return availableAction;
                    }
                }
            }

            return availableActions[rng.RandomInteger() % availableActions.size()];
        }
    }

    return action;
}

bool TryDetectFrameProactivity(const TScenarioRunRequestWrapper& requestWrapper, bool modalModeEnabled, IRng& rng, TClassificationResult* classificationResult) {
    if (const auto frame = requestWrapper.Input().FindSemanticFrame(FRAME_PROACTIVITY_ALICE_DO)) {
        *classificationResult->MutableRecognizedFrame() = *frame;
    } else if (const auto frame = requestWrapper.Input().FindSemanticFrame(FRAME_PROACTIVITY_BORED)) {
        *classificationResult->MutableRecognizedFrame() = *frame;
    } else {
        return false;
    }

    const auto* actions = FRAME_TO_ACTION_MAP.FindPtr(classificationResult->GetRecognizedFrame().GetName());
    if (!actions) {
        classificationResult->ClearRecognizedFrame();
        return false;
    }

    TVector<const TActionInfo*> allowedActions;
    for (const auto& actionInfo : *actions) {
        if (actionInfo.IsAllowed(requestWrapper, modalModeEnabled)) {
            allowedActions.push_back(&actionInfo);
        }
    }

    if (allowedActions.empty()) {
        classificationResult->ClearRecognizedFrame();
        return false;
    }

    const TActionInfo* choosenActionInfo = nullptr;
    if (const auto forcedProactivity = GetExperimentTypedValue<TString>(requestWrapper.ExpFlags(), EXP_HW_GCP_PROACTIVITY_FORCE_SOFT)) {
        for (const auto* actionInfo : allowedActions) {
            if (forcedProactivity.GetRef() == actionInfo->FrameActionName) {
                choosenActionInfo = actionInfo;
                break;
            }
        }
    }
    if (!choosenActionInfo) {
        choosenActionInfo = allowedActions[rng.RandomInteger() % allowedActions.size()];
    }

    classificationResult->MutableReplyInfo()->SetIntent(ToString(choosenActionInfo->FrameActionName));
    classificationResult->MutableReplyInfo()->MutableProactivityReply();
    classificationResult->SetIsFrameFeatured(true);

    const static TVector<TStringBuf> FRAMES_WITH_ENTITY_SEARCH = {FRAME_MOVIE_DISCUSS_SPECIFIC, FRAME_MOVIE_DISCUSS};
    if (FindPtr(FRAMES_WITH_ENTITY_SEARCH, choosenActionInfo->FrameActionName) && classificationResult->GetIsEntitySearchRequestAllowed()) {
        classificationResult->SetHasEntitySearchRequest(true);
    }

    if (choosenActionInfo->FrameActionName != FRAME_MOVIE_DISCUSS) {
        classificationResult->SetHasSearchSuggestsRequest(true);
    } else if (!requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SUGGESTS)) {
        classificationResult->SetHasSearchSuggestsRequest(true);
    }

    if (choosenActionInfo->FrameActionName != FRAME_MOVIE_DISCUSS) {
        classificationResult->SetHasSearchSuggestsRequest(true);
    } else if (!requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SUGGESTS)) {
        classificationResult->SetHasSearchSuggestsRequest(true);
    }

    return true;
}

void PatchReplyWithEntity(TGeneralConversationRunContextWrapper& contextWrapper, const TString& frameName, TReplyInfo* replyInfo) {
    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["frame"] = frameName;
    nlgData.Context["intent"] = replyInfo->GetIntent();
    if (replyInfo->GetEntityInfo().GetEntity().GetEntityCase() == TEntity::EntityCase::kMovie) {
        auto& movie = replyInfo->GetEntityInfo().GetEntity().GetMovie();
        nlgData.Context["movie_title"] = movie.GetTitle();
        nlgData.Context["movie_type"] = movie.GetType();
    }

    const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_PROACTIVITY_TEXT, nlgData);
    replyInfo->SetRenderedText(renderedPhrase.Text);
    replyInfo->SetRenderedVoice(renderedPhrase.Voice);
}

void AddProactivityActions(const TReplyInfo& replyInfo, TResponseBodyBuilder* responseBodyBuilder) {
    if (replyInfo.GetIntent() == FRAME_MOVIE_DISCUSS) {
        {
            NScenarios::TFrameAction action;
            action.MutableNluHint()->SetFrameName(replyInfo.GetIntent());
            responseBodyBuilder->AddAction("movie_discuss_action", std::move(action));
        }
        {
            NScenarios::TFrameAction action;
            action.MutableNluHint()->SetFrameName(ToString(FRAME_I_DONT_KNOW));
            responseBodyBuilder->AddAction("movie_discuss_i_dont_know_action", std::move(action));
        }
    } else if (replyInfo.GetIntent() == FRAME_GAME_DISCUSS) {
        NScenarios::TFrameAction action;
        action.MutableNluHint()->SetFrameName(replyInfo.GetIntent());
        responseBodyBuilder->AddAction("game_discuss_action", std::move(action));
    } else if (replyInfo.GetIntent() == FRAME_MUSIC_DISCUSS) {
        NScenarios::TFrameAction action;
        action.MutableNluHint()->SetFrameName(replyInfo.GetIntent());
        responseBodyBuilder->AddAction("music_discuss_action", std::move(action));
    } else if (replyInfo.GetIntent() == FRAME_MOVIE_DISCUSS_SPECIFIC) {
        {
            NScenarios::TFrameAction action;
            action.MutableNluHint()->SetFrameName(ToString(FRAME_YES_I_WATCH_IT));
            responseBodyBuilder->AddAction("movie_discuss_specific_watched_action", std::move(action));
        }
        {
            NScenarios::TFrameAction action;
            action.MutableFrame()->SetName(ToString(FRAME_YES_I_WATCH_IT));
            action.MutableNluHint()->SetFrameName(ToString(FRAME_PROACTIVITY_AGREE));
            responseBodyBuilder->AddAction("movie_discuss_specific_proactivity_agree", std::move(action));
        }
        {
            NScenarios::TFrameAction action;
            action.MutableNluHint()->SetFrameName(ToString(FRAME_NO_I_DID_NOT_WATCH_IT));
            responseBodyBuilder->AddAction("movie_discuss_specific_not_watched_action", std::move(action));
        }
    } else if (replyInfo.GetIntent() == FRAME_MOVIE_AKINATOR) {
        TFrame frame(replyInfo.GetIntent());
        frame.AddSlot({"content_type", "movie_suggest_content_type", TSlot::TValue{"movie"}});

        NScenarios::TFrameAction action;
        *action.MutableCallback() = ToCallback(frame.ToProto());
        action.MutableNluHint()->SetFrameName(ToString(FRAME_PROACTIVITY_AGREE));

        responseBodyBuilder->AddAction("proactivity_agree_action", std::move(action));
    } else {
        TFrame frame(replyInfo.GetIntent());
        NScenarios::TFrameAction action;
        *action.MutableFrame() = frame.ToProto();
        action.MutableNluHint()->SetFrameName(ToString(FRAME_PROACTIVITY_AGREE));
        responseBodyBuilder->AddAction("proactivity_agree_action", std::move(action));
    }
}

void AddProactivitySuggests(const TScenarioRunRequestWrapper& requestWrapper, const TReplyInfo& replyInfo,
        TNlgWrapper& nlgWrapper, TRTLogger& logger, TResponseBodyBuilder* responseBodyBuilder)
{
    const static TVector<TStringBuf> INTENTS_WITHOUT_PROACTIVITY_SUGGEST = {FRAME_MOVIE_DISCUSS, FRAME_MOVIE_DISCUSS_SPECIFIC, FRAME_GAME_DISCUSS, FRAME_MUSIC_DISCUSS};
    if (FindPtr(INTENTS_WITHOUT_PROACTIVITY_SUGGEST, replyInfo.GetIntent())) {
        return;
    }

    const auto& renderedPhrase = nlgWrapper.RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_PROACTIVITY_SUGGEST, TNlgData{logger, requestWrapper}).Text;
    const auto forceGcResponse = replyInfo.GetIntent() != FRAME_MOVIE_AKINATOR;
    AddSuggest("suggest_proactivity", renderedPhrase, ToString(SUGGEST_PROACTIVITY_TYPE), forceGcResponse, *responseBodyBuilder);
}

void AddProactivityAnalytics(const TReplyInfo& replyInfo, const TString& frameName, NScenarios::NGeneralConversation::TGCResponseInfo* gcResponseInfo) {
    auto& analyticsProactivityInfo = *gcResponseInfo->MutableProactivityInfo();
    analyticsProactivityInfo.SetFrameName(frameName);
    analyticsProactivityInfo.SetActionName(replyInfo.GetIntent());
    analyticsProactivityInfo.SetEntityKey(GetEntityKey(replyInfo.GetEntityInfo().GetEntity()));
}

} // namespace NAlice::NHollywood::NGeneralConversation
