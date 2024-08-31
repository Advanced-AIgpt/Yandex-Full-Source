#include "seq2seq_candidates.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/search/utils/serp_helpers.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/iterator/zip.h>

#include <util/generic/algorithm.h>

namespace NAlice::NHollywood::NGeneralConversation {
namespace {

constexpr TStringBuf REPLY_SEQ2SEQ_CANDIDATES_REQUEST_ITEM = "hw_reply_candidates_seq2seq_request";
constexpr TStringBuf REPLY_SEQ2SEQ_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM = "hw_reply_candidates_seq2seq_request_rtlog_token";
constexpr TStringBuf REPLY_SEQ2SEQ_CANDIDATES_RESPONSE_ITEM = "hw_reply_candidates_seq2seq_response";

TMaybe<TString> ExtractSnippetFromSummarization(TGeneralConversationRunContextWrapper& contextWrapper) {
    const auto summarization = contextWrapper.RequestWrapper().GetDataSource(EDataSourceType::WEB_SEARCH_SUMMARIZATION);
    if (!summarization) {
        return Nothing();
    }
    const auto snippets = summarization->GetWebSearchSummarization().GetSnippets();
    if (!snippets) {
        return Nothing();
    }
    const auto factSnippets = JsonFromString(Base64Decode(snippets)).GetArray();
    for (const auto& snippet : factSnippets) {
        if (!snippet.GetArray()) {
            continue;
        }
        const auto candidate = snippet[0].GetString();
        TVector<TStringBuf> _;
        if (Split(candidate, " ", _) > 3) {
            return candidate;
        }
    }
    return Nothing();
}

TMaybe<TString> ExtractSnippetFromDocs(TGeneralConversationRunContextWrapper& contextWrapper) {
    const auto searchDocs = contextWrapper.RequestWrapper().GetDataSource(EDataSourceType::WEB_SEARCH_SUMMARIZATION);
    if (!searchDocs) {
        return Nothing();
    }
    size_t pos;
    const auto snippet =  NAlice::NHollywood::NSearch::FindFactoidInDocs(searchDocs->GetWebSearchDocs(), "voiced_snippet", 10, NAlice::NHollywood::NSearch::ESS_ALL, pos);
    if (!snippet || !snippet->Has("text") || !((*snippet)["text"].GetString())) {
        return Nothing();
    }
    return (*snippet)["text"].GetString();
}

TString ExtractSnippet(TGeneralConversationRunContextWrapper& contextWrapper) {
    auto snippet = ExtractSnippetFromSummarization(contextWrapper);
    if (snippet) {
        return *snippet;
    }
    LOG_INFO(contextWrapper.Logger()) << "Snippet from summarization not found";
    snippet = ExtractSnippetFromDocs(contextWrapper);
    if (snippet) {
        return *snippet;
    }
    LOG_INFO(contextWrapper.Logger()) << "Snippet from search docs not found";
    return {};
}

} // namespace

template <typename TContextWrapper>
void AddReplySeq2SeqCandidatesRequest(const TString& url, TContextWrapper& contextWrapper, NGenerativeBoltalka::Proto::TGenerativeRequest& bodyProto, int numHypos) {
    bodyProto.SetNumHypos(numHypos);
    bodyProto.MutableSeed()->SetValue(contextWrapper.RequestWrapper().BaseRequestProto().GetRandomSeed());

    LOG_INFO(contextWrapper.Logger()) << "Send Request to seq2seq: " << SerializeProtoText(bodyProto);

    THttpHeaders headers;
    headers.AddHeader("Content-Type", "application/protobuf");
    headers.AddHeader("Accept", "application/protobuf");

    const auto seq2seqRequest = PrepareHttpRequest(
        url,
        contextWrapper.Ctx()->RequestMeta,
        contextWrapper.Logger(),
        ToString(REPLY_SEQ2SEQ_CANDIDATES_REQUEST_ITEM),
        bodyProto.SerializeAsString(),
        NAppHostHttp::THttpRequest::Post,
        headers
    );

    AddHttpRequestItems(*contextWrapper.Ctx(), seq2seqRequest, REPLY_SEQ2SEQ_CANDIDATES_REQUEST_ITEM, REPLY_SEQ2SEQ_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM);
}

void AddReplySeq2SeqCandidatesRequest(const TString& url, TGeneralConversationRunContextWrapper& contextWrapper, size_t contextLength, bool addSearchInfo) {
    const auto dialogHistory = GetDialogHistory(contextWrapper.RequestWrapper());
    auto context = ConstructContext(dialogHistory, contextLength, GetUtterance(contextWrapper.RequestWrapper()));
    std::reverse(context.begin(), context.end());
    if (addSearchInfo) {
        const auto snippet = ExtractSnippet(contextWrapper);
        context.push_back(snippet);
    }

    NGenerativeBoltalka::Proto::TGenerativeRequest bodyProto;
    auto* bodyProtoContext = bodyProto.MutableContext();
    for (auto& c : context) {
        bodyProtoContext->Add(TString(c));
    }
    AddReplySeq2SeqCandidatesRequest(url, contextWrapper, bodyProto);
}

template <typename TContextWrapper>
void AddReplySeq2SeqCandidatesRequest(const TString& url, TContextWrapper& contextWrapper, const TString& request, int numHypos) {
    NGenerativeBoltalka::Proto::TGenerativeRequest bodyProto;
    auto* bodyProtoContext = bodyProto.MutableContext();
    bodyProtoContext->Add(TString(request));
    AddReplySeq2SeqCandidatesRequest(url, contextWrapper, bodyProto, numHypos);
}

template void AddReplySeq2SeqCandidatesRequest<TGeneralConversationRunContextWrapper>(const TString& url, TGeneralConversationRunContextWrapper& contextWrapper, NGenerativeBoltalka::Proto::TGenerativeRequest& bodyProto, int numHypos);
template void AddReplySeq2SeqCandidatesRequest<TGeneralConversationRunContextWrapper>(const TString& url, TGeneralConversationRunContextWrapper& contextWrapper, const TString& request, int numHypos);

template void AddReplySeq2SeqCandidatesRequest<TGeneralConversationApplyContextWrapper>(const TString& url, TGeneralConversationApplyContextWrapper& contextWrapper, NGenerativeBoltalka::Proto::TGenerativeRequest& bodyProto, int numHypos);
template void AddReplySeq2SeqCandidatesRequest<TGeneralConversationApplyContextWrapper>(const TString& url, TGeneralConversationApplyContextWrapper& contextWrapper, const TString& request, int numHypos);


TMaybe<TVector<TSeq2SeqReplyCandidate>> RetireReplySeq2SeqCandidatesResponse(const TScenarioHandleContext& ctx) {
    const auto seq2seqResponseMaybe = RetireHttpResponseProtoMaybe<NGenerativeBoltalka::Proto::TGenerativeResponse>(ctx, REPLY_SEQ2SEQ_CANDIDATES_RESPONSE_ITEM, REPLY_SEQ2SEQ_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM);
    if (!seq2seqResponseMaybe) {
        LOG_INFO(ctx.Ctx.Logger()) << "Receive Nothing from seq2seq.";
        return Nothing();
    }

    LOG_INFO(ctx.Ctx.Logger()) << "Receive from seq2seq: " << SerializeProtoText(seq2seqResponseMaybe.GetRef());

    TVector<TSeq2SeqReplyCandidate> result;
    for (const auto& r : seq2seqResponseMaybe->GetResponses()) {
        TSeq2SeqReplyCandidate candidate;
        candidate.SetText(r.GetResponse());
        candidate.SetRelevance(r.GetScore());
        candidate.SetNumTokens(r.GetNumTokens());
        result.push_back(std::move(candidate));
    }

    return result;
}

} // namespace NAlice::NHollywood::NGeneralConversation
