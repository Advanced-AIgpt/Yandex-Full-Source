#include "movie_akinator_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>

#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NGeneralConversation {

TMovieAkinatorRenderStrategy::TMovieAkinatorRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , AkinatorSuggestsState_()
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kMovieAkinatorReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: MovieAkinatorRenderStrategy";
}

void TMovieAkinatorRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper) {
    const auto& clusteredMovies = ContextWrapper_.Resources().GetClusteredMovies();

    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};

    TAkinatorStateWrapper akinatorState(TAkinatorState::FromProto(responseWrapper->SessionState.GetMovieAkinatorState()),
                                        &AkinatorSuggestsState_);

    TAkinatorResponseBuilder akinatorResponseBuilder(clusteredMovies, ClassificationResult_.GetRecognizedFrame(),
                                                     ContextWrapper_.RequestWrapper(), akinatorState, nlgData, ContextWrapper_.NlgWrapper(),
                                                     responseWrapper->Builder, ContextWrapper_.Logger(), ContextWrapper_.Rng());

    akinatorResponseBuilder.Build();

    *responseWrapper->SessionState.MutableMovieAkinatorState() = akinatorState.BaseState.ToProto<TMovieAkinatorState>();
    *responseWrapper->GcResponseInfo.MutableMovieAkinatorInfo() = akinatorResponseBuilder.GetResponseAnalyticsInfo();

    if (akinatorState.DiscussableEntityId) {
        if (const auto* entityPtr = ContextWrapper_.Resources().GetEntity(*akinatorState.DiscussableEntityId)) {
            auto& entityDiscussion = *responseWrapper->SessionState.MutableEntityDiscussion();
            *entityDiscussion.MutableEntity() = *entityPtr;
            if (ContextWrapper_.Rng().RandomDouble() < entityPtr->GetMovie().GetNegativeAnswerFraction()) {
                entityDiscussion.SetDiscussionSentiment(TEntityDiscussion::NEGATIVE);
            } else {
                entityDiscussion.SetDiscussionSentiment(TEntityDiscussion::POSITIVE);
            }
            UpdateLastDiscussion(ClassificationResult_, &responseWrapper->SessionState);
            return;
        }
    }

    responseWrapper->SessionState.ClearEntityDiscussion();
}

void TMovieAkinatorRenderStrategy::AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) {
    for (const auto& suggest : AkinatorSuggestsState_.Suggests) {
        if (suggest.HasPredefinedEffect) {
            responseWrapper->Builder.GetResponseBodyBuilder()->AddActionSuggest(suggest.SuggestId).Title(suggest.SuggestTitle);
        } else {
            AddSuggest(suggest.SuggestId, suggest.SuggestTitle, ToString(SUGGEST_TYPE),
                       /* forceGcResponse= */ false, *responseWrapper->Builder.GetResponseBodyBuilder());
        }
    }
}

} // namespace NAlice::NHollywood::NGeneralConversation
