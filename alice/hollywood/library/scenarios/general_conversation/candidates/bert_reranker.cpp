#include "bert_reranker.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/boltalka/generative/service/proto/bert_request.pb.h>
#include <alice/boltalka/generative/service/proto/bert_response.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {
namespace {

constexpr TStringBuf BERT_RERANKER_REQUEST_ITEM = "hw_bert_reranker_request";
constexpr TStringBuf BERT_RERANKER_REQUEST_RTLOG_TOKEN_ITEM = "hw_bert_reranker_request_rtlog_token";
constexpr TStringBuf BERT_RERANKER_RESPONSE_ITEM = "hw_bert_reranker_response";

} // namespace

void AddBertRerankerRequest(const TAggregatedRepliesState& repliesState, TGeneralConversationRunContextWrapper& contextWrapper, size_t contextLength) {
    if (repliesState.GetReplyCandidates().empty()) {
        return;
    }

    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const auto dialogHistory = GetDialogHistory(requestWrapper);
    const auto context = ConstructContext(dialogHistory, contextLength, GetUtterance(requestWrapper));

    NGenerativeBoltalka::Proto::TBertFactorRequest requestProto;
    auto* protoContext = requestProto.MutableContext();
    for (const auto& c : context) {
        protoContext->Add(TString(c));
    }
    auto* requestProtoCandidates = requestProto.MutableCandidates();
    for (const auto& c : repliesState.GetReplyCandidates()) {
        requestProtoCandidates->Add(GetAggregatedReplyText(c));
    }

    THttpHeaders headers;
    headers.AddHeader("Content-Type", "application/protobuf");
    headers.AddHeader("Accept", "application/protobuf");

    const TSessionState sessionState = GetOnlyProtoOrThrow<TSessionState>(contextWrapper.Ctx()->ServiceCtx, STATE_SESSION);
    const auto bertUrl = GetExperimentTypedValue<TString>(requestWrapper, sessionState, EXP_HW_GC_BERT_URL).GetOrElse(ToString(DEFAULT_BERT_URL));

    const auto bertRequest = PrepareHttpRequest(
        bertUrl,
        contextWrapper.Ctx()->RequestMeta,
        contextWrapper.Logger(),
        ToString(BERT_RERANKER_REQUEST_ITEM),
        requestProto.SerializeAsString(),
        NAppHostHttp::THttpRequest::Post,
        headers
    );

    AddHttpRequestItems(*contextWrapper.Ctx(), bertRequest, BERT_RERANKER_REQUEST_ITEM, BERT_RERANKER_REQUEST_RTLOG_TOKEN_ITEM);
}

bool RetireBertRerankerResponse(const TScenarioHandleContext& ctx, TVector<TAggregatedReplyCandidate>* candidates) {
    const auto bertResponseMaybe = RetireHttpResponseProtoMaybe<NGenerativeBoltalka::Proto::TBertFactorResponse>(ctx, BERT_RERANKER_RESPONSE_ITEM, BERT_RERANKER_REQUEST_RTLOG_TOKEN_ITEM);
    if (!bertResponseMaybe) {
        return false;
    }

    Y_ENSURE(candidates->size() == bertResponseMaybe->CandidatesScoresSize());
    for (size_t i = 0; i < candidates->size(); ++i) {
        *(*candidates)[i].MutableBertOutput() = bertResponseMaybe->GetCandidatesScores(i);
    }
    return true;
}

} // namespace NAlice::NHollywood::NGeneralConversation
