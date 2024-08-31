#include "proactivity_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/proactivity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/iterator/enumerate.h>

#include <util/random/shuffle.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

TVector<TString> GetMovieSuggests(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState, const TReplyInfo& replyInfo) {
    const auto& resources = contextWrapper.Resources();
    auto& rng = contextWrapper.Rng();

    const auto& movieType = replyInfo.GetEntityInfo().GetEntity().GetMovie().GetType();
    const auto* entitySearchCache = GetEntitySearchCache(replyInfo, sessionState);
    THashSet<TString> discussed(sessionState.GetRecentDiscussedEntities().begin(), sessionState.GetRecentDiscussedEntities().end());
    TVector<TString> movieSuggests;
    const auto addToSuggests = [&] (const TString& entityKey) {
        if (const auto* entityPtr = resources.GetEntity(entityKey)) {
            if (discussed.contains(entityKey)) {
                return;
            }
            if (entityPtr->GetMovie().GetType() != movieType) {
                return;
            }
            movieSuggests.push_back(entityPtr->GetMovie().GetTitle());
            discussed.insert(entityKey);
        }
    };

    if (entitySearchCache) {
        for (const auto& entityKey : entitySearchCache->GetEntityKeys()) {
            if (movieSuggests.size() >= MAX_SUGGESTS_SIZE) {
                break;
            }
            addToSuggests(entityKey);
        }
    }

    const auto& moviesToDiscuss = resources.GetMoviesToDiscussByType(movieType);
    TVector<const TMovie*> moviesToDiscussShuffled;
    moviesToDiscussShuffled.reserve(moviesToDiscuss.size());
    for (const auto& movie : moviesToDiscuss) {
        moviesToDiscussShuffled.push_back(&movie);
    }
    ShuffleRange(moviesToDiscussShuffled, rng);
    for (const auto* movie : moviesToDiscussShuffled) {
        if (movieSuggests.size() >= MAX_SUGGESTS_SIZE) {
            break;
        }
        const auto entityKey = GetMovieEntityKey(movie->GetId());
        addToSuggests(entityKey);
    }

    ShuffleRange(movieSuggests, rng);
    return movieSuggests;
}

} // namespace

TProactivityRenderStrategy::TProactivityRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyInfo_(replyInfo)
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kProactivityReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: ProactivityRenderStrategy";
}

void TProactivityRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper) {
    AddProactivityActions(ReplyInfo_, responseWrapper->Builder.GetResponseBodyBuilder());

    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["rendered_text"] = ReplyInfo_.GetRenderedText();
    nlgData.Context["rendered_voice"] = ReplyInfo_.GetRenderedVoice();
    if (ReplyInfo_.GetTtsSpeed() != 1.0) {
        nlgData.Context["tts_speed"] = ReplyInfo_.GetTtsSpeed();
    }

    responseWrapper->Builder.GetResponseBodyBuilder()->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_RESULT, /* buttons = */ {}, nlgData);

    responseWrapper->GcResponseInfo.SetSource(ReplyInfo_.GetProactivityReply().GetNlgSearchReply().GetSource());
    AddProactivityAnalytics(ReplyInfo_, ClassificationResult_.GetRecognizedFrame().GetName(), &responseWrapper->GcResponseInfo);

    UpdateLastDiscussion(ClassificationResult_, &responseWrapper->SessionState);

    responseWrapper->SessionState.SetLastProactivityRequestServerTimeMs(ClassificationResult_.GetCurrentRequestServerTimeMs());
}

void TProactivityRenderStrategy::AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) {
    AddProactivitySuggests(ContextWrapper_.RequestWrapper(), ReplyInfo_, ContextWrapper_.NlgWrapper(), ContextWrapper_.Logger(), responseWrapper->Builder.GetResponseBodyBuilder());

    const bool isMovieDisscussionAllowedByDefault = IsMovieDisscussionAllowedByDefault(ContextWrapper_.RequestWrapper(), responseWrapper->SessionState.GetModalModeEnabled());
    const bool isSuggestsAllowed = isMovieDisscussionAllowedByDefault || ContextWrapper_.RequestWrapper().HasExpFlag(EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SUGGESTS);
    if (isSuggestsAllowed) {
        if (ReplyInfo_.GetIntent() == FRAME_MOVIE_DISCUSS) {
            for (const auto& [suggestNumber, phrase] : Enumerate(GetMovieSuggests(ContextWrapper_, responseWrapper->SessionState, ReplyInfo_))) {
                AddSuggest("suggest_movie_" + ToString(suggestNumber), phrase, ToString(SUGGEST_TYPE),
                            /* forceGcResponse= */ true, *responseWrapper->Builder.GetResponseBodyBuilder());
            }
        }
    }
}


} // namespace NAlice::NHollywood::NGeneralConversation
