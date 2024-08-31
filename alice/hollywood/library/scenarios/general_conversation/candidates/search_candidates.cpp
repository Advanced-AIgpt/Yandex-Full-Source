#include "search_candidates.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_params.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/boltalka/libs/factors/proto/nlgsearch_factors.pb.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/iterator/zip.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/algorithm.h>

namespace NAlice::NHollywood::NGeneralConversation {
namespace {

constexpr TStringBuf REPLY_CANDIDATES_REQUEST_ITEM = "hw_reply_candidates_search_request";
constexpr TStringBuf REPLY_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM = "hw_reply_candidates_search_request_rtlog_token";
constexpr TStringBuf REPLY_CANDIDATES_SEARCH_RESPONSE_ITEM = "hw_reply_candidates_search_response";
constexpr TStringBuf SUGGEST_CANDIDATES_REQUEST_ITEM = "hw_suggest_candidates_search_request";
constexpr TStringBuf SUGGEST_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM = "hw_suggest_candidates_search_request_rtlog_token";
constexpr TStringBuf SUGGEST_CANDIDATES_SEARCH_RESPONSE_ITEM = "hw_suggest_candidates_search_response";

TVector<TGif> ParseSearchGifs(const TGif& gif) {
    if (!gif.GetUrl()) {
        return {};
    }
    TVector<TString> urls = StringSplitter(gif.GetUrl()).Split(' ');
    TVector<TString> sourceUrls = StringSplitter(gif.GetSourceUrl()).Split(' ');
    TVector<TString> sourceTexts = StringSplitter(gif.GetSourceText()).Split(' ');
    if  (urls.size() != sourceUrls.size() || urls.size() != sourceTexts.size() || urls.empty()) {
        return {};
    }
    TVector<TGif> gifCards;
    gifCards.reserve(urls.size());
    for (const auto& [url, sourceUrl, sourceText] : Zip(urls, sourceUrls, sourceTexts)) {
        TGif gif;
        gif.SetUrl(url);
        gif.SetSourceUrl(sourceUrl);
        gif.SetSourceText(sourceText);
        gifCards.push_back(gif);
    }
    return gifCards;
}

TVector<TNlgSearchReplyCandidate> ExtractCandidates(const NJson::TJsonValue& docs, bool preferChildReply, TRTLogger& logger) {
    TVector<TNlgSearchReplyCandidate> candidates;
    LOG_INFO(logger) << docs;

    for (const auto& grouping : docs["Grouping"].GetArray()) {
        for (const auto& group : grouping["Group"].GetArray()) {
            for (const auto& doc : group["Document"].GetArray()) {
                TNlgSearchReplyCandidate candidate;
                candidate.SetRelevance(doc["Relevance"].GetDouble());
                candidate.SetDocId(doc["DocId"].GetUInteger());
                bool isSeq2SeqResult = false;

                for (const auto& attr : doc["FirstStageAttribute"].GetArray()) {
                    if (attr["Key"].GetString() == "_NlgSearchFactorsResult" && !attr["Value"].GetString().Empty()) {
                        auto factors = ParseProto<NAlice::NBoltalka::TNlgSearchFactors>(Base64Decode(attr["Value"].GetString()));
                        *candidate.MutableFactors() = std::move(factors);
                    } else if (attr["Key"].GetString() == "_Seq2SeqResult" && !attr["Value"].GetString().Empty()) {
                        candidate.SetText(PostProcessCandidateText(attr["Value"].GetString()));
                        candidate.SetSource(ToString(SOURCE_SEQ2SEQ));
                        isSeq2SeqResult = true;
                    }
                }
                if (!isSeq2SeqResult) {
                    TGif searchGif;
                    TString reply;
                    TString disrespectReply;
                    for (const auto& attr : doc["ArchiveInfo"]["GtaRelatedAttribute"].GetArray()) {
                        if (attr["Key"] == "reply") {
                            reply = PostProcessCandidateText(attr["Value"].GetString());
                        } else if (attr["Key"] == "disrespect_reply") {
                            disrespectReply = PostProcessCandidateText(attr["Value"].GetString());
                        } else if (attr["Key"] == "source") {
                            candidate.SetSource(attr["Value"].GetString());
                        } else if (attr["Key"] == "proactivity_action") {
                            candidate.SetAction(attr["Value"].GetString());
                        } else if (attr["Key"] == "gif_url") {
                            searchGif.SetUrl(attr["Value"].GetString());
                        } else if (attr["Key"] == "gif_source_url") {
                            searchGif.SetSourceUrl(attr["Value"].GetString());
                        } else if (attr["Key"] == "gif_source_text") {
                            searchGif.SetSourceText(attr["Value"].GetString());
                        }
                    }
                    if (preferChildReply && disrespectReply) {
                        candidate.SetText(disrespectReply);
                    } else {
                        candidate.SetText(reply);
                    }
                    for (const auto& gifCard : ParseSearchGifs(searchGif)) {
                        (*candidate.AddGifs()) = gifCard;
                    }
                }

                if (candidate.GetText()) {
                    candidates.emplace_back(candidate);
                }
            }
        }
    }

    return candidates;
}

TString ConstructReplyCandidatesUrl(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState,
        const TClassificationResult& classificationResult, size_t contextLength)
{
    const auto dialogHistory = GetDialogHistory(contextWrapper.RequestWrapper());
    const auto context = ConstructContextString(dialogHistory, contextLength, GetUtterance(contextWrapper.RequestWrapper()));
    const auto params = ConstructReplyCandidatesParams(contextWrapper, context, sessionState, classificationResult);

    return params.ToString();
}

TString ConstructSuggestCandidatesUrl(const TScenarioRunRequestWrapper& requestWrapper, const TString& reply, size_t contextLength) {
    const auto dialogHistory = GetDialogHistory(requestWrapper);
    const auto context = ConstructContextString(dialogHistory, contextLength, GetUtterance(requestWrapper), reply);
    const auto params = ConstructSuggestCandidatesParams(context, requestWrapper);

    return params.ToString();
}

} // namespace

namespace {

} // namespace

void AddReplyCandidatesRequest(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState,
        const TClassificationResult& classificationResult, size_t contextLength)
{
    const auto url = ConstructReplyCandidatesUrl(contextWrapper, sessionState, classificationResult, contextLength);
    const auto request = PrepareHttpRequest(url, contextWrapper.Ctx()->RequestMeta, contextWrapper.Logger(), ToString(REPLY_CANDIDATES_REQUEST_ITEM));
    AddHttpRequestItems(*contextWrapper.Ctx(), request, REPLY_CANDIDATES_REQUEST_ITEM, REPLY_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM);
}

TVector<TNlgSearchReplyCandidate> RetireReplyCandidatesResponse(const TScenarioHandleContext& ctx, bool preferChildReply) {
    const auto jsonResponse = RetireHttpResponseJson(ctx, REPLY_CANDIDATES_SEARCH_RESPONSE_ITEM, REPLY_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM, /* logBody */ false);

    return ExtractCandidates(jsonResponse, preferChildReply, ctx.Ctx.Logger());
}

void AddSuggestCandidatesRequest(const TScenarioRunRequestWrapper& requestWrapper, const TString& reply, size_t contextLength, TScenarioHandleContext* ctx) {
    const auto url = ConstructSuggestCandidatesUrl(requestWrapper, reply, contextLength);
    const auto request = PrepareHttpRequest(url, ctx->RequestMeta, ctx->Ctx.Logger(), ToString(SUGGEST_CANDIDATES_REQUEST_ITEM));
    AddHttpRequestItems(*ctx, request, SUGGEST_CANDIDATES_REQUEST_ITEM, SUGGEST_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM);
}

TMaybe<TVector<TNlgSearchReplyCandidate>> RetireSuggestCandidatesResponse(const TScenarioHandleContext& ctx, bool preferChildSuggests) {
    const auto jsonResponseMaybe = RetireHttpResponseJsonMaybe(ctx, SUGGEST_CANDIDATES_SEARCH_RESPONSE_ITEM, SUGGEST_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM);
    if (jsonResponseMaybe) {
        auto candidates = ExtractCandidates(jsonResponseMaybe.GetRef(), preferChildSuggests, ctx.Ctx.Logger());
        DeduplicateCandidates(&candidates);
        return candidates;
    }

    return Nothing();
}

void UpdateUsedRepliesState(const TScenarioRunRequestWrapper& requestWrapper, const TString& text, TSessionState* sessionState) {
    auto* currentReplyInfo = sessionState->AddUsedRepliesInfo();
    currentReplyInfo->SetHash(THash<TString>{}(text));
    if (requestWrapper.HasExpFlag(EXP_HW_ENABLE_GC_TEXT_IN_STATE)) {
        currentReplyInfo->SetText(text);
    }

    const size_t replyHistorySize = sessionState->GetUsedRepliesInfo().size();
    if (replyHistorySize > MAX_REPLY_HISTORY_SIZE) {
        const size_t oldRepliesIndex = replyHistorySize - MAX_REPLY_HISTORY_SIZE;
        auto& history = *sessionState->MutableUsedRepliesInfo();
        history.erase(history.begin(), history.begin() + oldRepliesIndex);
    }
}


} // namespace NAlice::NHollywood::NGeneralConversation
