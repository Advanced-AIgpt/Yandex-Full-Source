#include "continue_render_handle.h"

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

void TGeneralConversationContinueRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    TGeneralConversationApplyContextWrapper contextWrapper(&ctx);

    const auto classificationResult = GetOnlyProtoOrThrow<TClassificationResult>(ctx.ServiceCtx, STATE_CLASSIFICATION_RESULT);
    const auto sessionState = GetOnlyProtoOrThrow<TSessionState>(ctx.ServiceCtx, STATE_SESSION);
    auto replyState = GetMaybeOnlyProto<TReplyState>(ctx.ServiceCtx, STATE_REPLY);

    if (replyState && replyState->GetReplyInfo().HasNlgSearchReply()) {
        auto nlgSearchReply = replyState->GetReplyInfo().GetNlgSearchReply();
        *replyState->MutableReplyInfo()->MutableAggregatedReply()->MutableNlgSearchReply() = std::move(nlgSearchReply);
    }
    TGeneralConversationContinueResponseBuilder responseBuilder(contextWrapper, classificationResult, sessionState, replyState, Nothing());
    const auto response = std::move(responseBuilder).BuildContinueResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    if (auto responseBodyBuilder = responseBuilder.GetContinueResponseBodyBuilder()) {
        for (auto const& [cardId, cardData] : responseBodyBuilder->GetRenderData()) {
            LOG_DEBUG(ctx.Ctx.Logger()) << "Adding to context render_data with id " << cardId;
            ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
        }
    }
}

}  // namespace NAlice::NHollywood::NGeneralConversation
