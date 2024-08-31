#include "run_candidates_aggregator_handle.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/bert_reranker.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/seq2seq_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/library/logger/logger.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

void TGeneralConversationCandidatesAggregatorHandle::Do(TScenarioHandleContext& ctx) const {
    TGeneralConversationRunContextWrapper contextWrapper(&ctx);
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const auto classificationResult = GetOnlyProtoOrThrow<TClassificationResult>(ctx.ServiceCtx, STATE_CLASSIFICATION_RESULT);
    const TSessionState sessionState = GetOnlyProtoOrThrow<TSessionState>(ctx.ServiceCtx, STATE_SESSION);

    TAggregatedRepliesState repliesState;

    if (classificationResult.GetHasSearchReplyRequest()) {
        const bool preferChildReply = !requestWrapper.HasExpFlag(EXP_HW_GC_DISABLE_CHILD_REPLIES) && classificationResult.GetIsChildTalking();
        for (auto& candidate : RetireReplyCandidatesResponse(*contextWrapper.Ctx(), preferChildReply)) {
            *repliesState.AddReplyCandidates()->MutableNlgSearchReply() = std::move(candidate);
        }
        LOG_INFO(ctx.Ctx.Logger()) << "ReplyCandidates after search: " << repliesState.GetReplyCandidates().size();
    }

    if (classificationResult.GetHasSeq2SeqReplyRequest()) {
        if (auto seq2seqResponse = RetireReplySeq2SeqCandidatesResponse(ctx); seq2seqResponse.Defined()) {
            for (auto& candidate : seq2seqResponse.GetRef()) {
                *repliesState.AddReplyCandidates()->MutableSeq2SeqReply() = std::move(candidate);
            }
        }
        LOG_INFO(ctx.Ctx.Logger()) << "ReplyCandidates after seq2seq: " << repliesState.GetReplyCandidates().size();
    }

    AddBertRerankerRequest(repliesState, contextWrapper, GetDialogHistorySize(requestWrapper, sessionState));

    ctx.ServiceCtx.AddProtobufItem(repliesState, STATE_AGGREGATED_REPLIES);
}

}  // namespace NAlice::NHollywood::NGeneralConversation
