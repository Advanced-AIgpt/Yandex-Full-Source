#include "run_init_handle.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/phead_scorer.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/seq2seq_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/classification/frame_classifier.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/response/response_builder.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

constexpr ui64 ENTITY_DISSCUSSION_MEMORY = 2;
constexpr ui64 ENTITY_SEARCH_CACHE_TIMEOUT = 24*60*60*1000;;

void ResetDiscussion(TSessionState* sessionState) {
    sessionState->ClearEntityDiscussion();
}

void ResetSessionState(TSessionState* sessionState) {
    sessionState->SetModalModeEnabled(false);
}

void ResetEntitySearchCache(TSessionState* sessionState) {
    sessionState->ClearEntitySearchCache();
}

TSessionState UnpackSession(const TScenarioRunRequestWrapper& requestWrapper, TRTLogger& logger) {
    TSessionState sessionState;
    requestWrapper.Proto().GetBaseRequest().GetState().UnpackTo(&sessionState);
    LOG_INFO(logger) << "Session state in request: " << SerializeProtoText(sessionState);

    bool isNewSession = requestWrapper.BaseRequestProto().GetIsNewSession();

    if (isNewSession && requestWrapper.HasExpFlag(EXP_HW_GC_ENABLE_MODALITY_IN_PURE_GC)) {
        ResetSessionState(&sessionState);
    }

    if (requestWrapper.HasExpFlag(EXP_HW_GC_FORCE_PURE_GC)) {
        sessionState.SetModalModeEnabled(true);
        sessionState.SetLastRequestServerTimeMs(GetServerTimeMs(requestWrapper));
    }

    const auto discussionMemory = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_GC_DISCUSSION_MEMORY).GetOrElse(ENTITY_DISSCUSSION_MEMORY);
    if (sessionState.GetEntityDiscussion().GetLastSequenceNumber() + discussionMemory <= sessionState.GetLastRequestSequenceNumber()) {
        ResetDiscussion(&sessionState);
    }

    const auto currentServerTimeMs = GetServerTimeMs(requestWrapper);
    const auto entitySearchCacheTimeout = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_GC_ENTITY_SEARCH_CACHE_TIMEOUT).GetOrElse(ENTITY_SEARCH_CACHE_TIMEOUT);
    if (sessionState.GetEntitySearchCache().GetLastUpdateTimeMs() + entitySearchCacheTimeout < currentServerTimeMs) {
        ResetEntitySearchCache(&sessionState);
    }

    if (!CountForEasterEggSuggest(requestWrapper)) {
        sessionState.ClearEasterEggState();
    }

    if (!sessionState.GetIsHeavyScenario()) {
        if (requestWrapper.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS)) {
            sessionState.SetIsHeavyScenario(true);
        }
    }

    LOG_INFO(logger) << "Session state in request after unpack: " << SerializeProtoText(sessionState);
    return sessionState;
}

} // namespace

void TGeneralConversationInitHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    TGeneralConversationRunContextWrapper contextWrapper(&ctx);
    const auto& requestWrapper = contextWrapper.RequestWrapper();

    const TSessionState sessionState = UnpackSession(requestWrapper, ctx.Ctx.Logger());

    const auto& resources = ctx.Ctx.ScenarioResources<TGeneralConversationResources>();
    const auto& fastData = contextWrapper.FastData();
    const TFrameClassifier frameClassifier(resources, fastData, requestWrapper, *this, sessionState);

    auto classificationResult = frameClassifier.ClassifyRequest(ctx);
    if (requestWrapper.HasExpFlag(EXP_HW_GC_DEBUG_DISABLE_SEARCH_SUGGESTS)) {
        classificationResult.SetHasSearchSuggestsRequest(false);
    }

    if (const auto* dialogHistory = requestWrapper.GetDataSource(EDataSourceType::DIALOG_HISTORY)) {
        LOG_INFO(ctx.Ctx.Logger()) << "Dialog history: " << SerializeProtoText(*dialogHistory);
    }
    LOG_INFO(ctx.Ctx.Logger()) << "Classification result: " << SerializeProtoText(classificationResult);

    if (classificationResult.GetHasGenerativeToastRequest()) {
        AddReplySeq2SeqCandidatesRequest("/generative_toast", contextWrapper, classificationResult.GetReplyInfo().GetGenerativeToastReply().GetTopic());
    }

    if (classificationResult.GetHasSeq2SeqReplyRequest()) {
        auto seq2seqUrl = GetExperimentTypedValue<TString>(requestWrapper, sessionState, EXP_HW_GC_SEQ2SEQ_URL).GetOrElse(ToString(DEFAULT_SEQ2SEQ_URL));
        bool useSearch = sessionState.GetIsHeavyScenario() && !requestWrapper.HasExpFlag(EXP_HW_DO_NOT_USE_SEARCH_IN_HEAVY);
        if (classificationResult.GetUserLanguage() == ELang::L_ARA) {
            seq2seqUrl = GetExperimentTypedValue<TString>(requestWrapper, sessionState, EXP_HW_GC_SEQ2SEQ_URL).GetOrElse(ToString(DEFAULT_ARABOBA_URL));
            useSearch = false;
            const float banClfThreshold = GetExperimentTypedValue<float>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_ARABOBA_BAN_CLF_THRESHOLD_PREFIX).GetOrElse(DEFAULT_ARABOBA_BAN_CLF_THRESHOLD);
            if (banClfThreshold < ARABOBA_BAN_CLF_MAX_SCORE) {
                const auto banClfUrl = GetExperimentTypedValue<TString>(requestWrapper, sessionState, EXP_HW_GC_ARABOBA_BAN_CLF_URL).GetOrElse(ToString(DEFAULT_ARABOBA_BAN_CLF_URL));
                const auto pheadPath = GetExperimentTypedValue<TString>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_ARABOBA_BAN_CLF_PHEAD_PATH).GetOrElse(ToString(DEFAULT_ARABOBA_BAN_CLF_PHEAD_PATH));
                AddPheadRequest(banClfUrl, contextWrapper, pheadPath);
            }
        }
        AddReplySeq2SeqCandidatesRequest(seq2seqUrl, contextWrapper, GetDialogHistorySize(requestWrapper, sessionState), useSearch);
    }

    if (classificationResult.GetHasSearchReplyRequest()) {
        AddReplyCandidatesRequest(contextWrapper, sessionState, classificationResult, GetDialogHistorySize(requestWrapper, sessionState));
    }

    if (classificationResult.GetHasSeq2SeqReplyRequest() || classificationResult.GetHasSearchReplyRequest()) {
        if (classificationResult.GetIsAggregatedRequest()) {
            ctx.ServiceCtx.AddProtobufItem(TAggregatedRepliesState{}, STATE_AGGREGATED_REQUEST);
        }
    }

    if (classificationResult.GetHasEntitySearchRequest()) {
        if (requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH)) {
            AddEntityCandidatesRequest(requestWrapper, &ctx);
        }
    }

    ctx.ServiceCtx.AddProtobufItem(classificationResult, STATE_CLASSIFICATION_RESULT);
    ctx.ServiceCtx.AddProtobufItem(sessionState, STATE_SESSION);
}

}  // namespace NAlice::NHollywood::NGeneralConversation
