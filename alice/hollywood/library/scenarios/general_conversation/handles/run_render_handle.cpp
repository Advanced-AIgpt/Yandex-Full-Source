#include "run_render_handle.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/general_conversation_response_builder.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

void TGeneralConversationRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    TGeneralConversationRunContextWrapper contextWrapper(&ctx);

    const auto classificationResult = GetOnlyProtoOrThrow<TClassificationResult>(ctx.ServiceCtx, STATE_CLASSIFICATION_RESULT);
    const auto sessionState = GetOnlyProtoOrThrow<TSessionState>(ctx.ServiceCtx, STATE_SESSION);
    auto replyState = GetMaybeOnlyProto<TReplyState>(ctx.ServiceCtx, STATE_REPLY);
    const bool preferChildSuggests = contextWrapper.RequestWrapper().HasExpFlag(EXP_HW_GC_ENABLE_CHILD_SUGGESTS);

    bool requiresSearchSuggest = RequiresSearchSuggests(contextWrapper.RequestWrapper(), replyState ? &(replyState->GetReplyInfo()) : nullptr, classificationResult);
    const auto suggestsState = requiresSearchSuggest ? RetireSuggestCandidatesResponse(ctx, preferChildSuggests) : Nothing();

    if (replyState && replyState->GetReplyInfo().HasNlgSearchReply()) {
        auto nlgSearchReply = replyState->GetReplyInfo().GetNlgSearchReply();
        *replyState->MutableReplyInfo()->MutableAggregatedReply()->MutableNlgSearchReply() = std::move(nlgSearchReply);
    }

    if (replyState && replyState->HasReplyInfo()) {
        replyState->MutableReplyInfo()->SetTtsSpeed(GetExperimentTypedValue<double>(contextWrapper.RequestWrapper(), sessionState, EXP_HW_SET_TTS_SPEED).GetOrElse(1.0));
    }

    TGeneralConversationRunResponseBuilder responseBuilder(contextWrapper, classificationResult, sessionState, replyState, suggestsState);

    const auto response = std::move(responseBuilder).BuildResponse(classificationResult.GetNeedContinue());
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    if (auto responseBodyBuilder = responseBuilder.GetRunResponseBodyBuilder()) {
        for (auto const& [cardId, cardData] : responseBodyBuilder->GetRenderData()) {
            LOG_DEBUG(ctx.Ctx.Logger()) << "Adding to context render_data with id " << cardId;
            ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
        }
    }
}

}  // namespace NAlice::NHollywood::NGeneralConversation
