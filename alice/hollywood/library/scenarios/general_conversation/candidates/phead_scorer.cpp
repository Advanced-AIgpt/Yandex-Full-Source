#include "phead_scorer.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/boltalka/generative/service/proto/phead.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {
namespace {

constexpr TStringBuf PHEAD_REQUEST_ITEM = "hw_phead_request";
constexpr TStringBuf PHEAD_REQUEST_RTLOG_TOKEN_ITEM = "hw_phead_request_rtlog_token";
constexpr TStringBuf PHEAD_RESPONSE_ITEM = "hw_phead_response";

} // namespace

void AddPheadRequest(const TString& url, TGeneralConversationRunContextWrapper& contextWrapper, const TString& pheadPath) {
    NGenerativeBoltalka::Proto::TPHeadRequest bodyProto;
    bodyProto.SetText(GetUtterance(contextWrapper.RequestWrapper()));
    bodyProto.SetPtuneYtPath(pheadPath);

    THttpHeaders headers;
    headers.AddHeader("Content-Type", "application/protobuf");
    headers.AddHeader("Accept", "application/protobuf");

    const auto request = PrepareHttpRequest(
        url,
        contextWrapper.Ctx()->RequestMeta,
        contextWrapper.Logger(),
        ToString(PHEAD_REQUEST_ITEM),
        bodyProto.SerializeAsString(),
        NAppHostHttp::THttpRequest::Post,
        headers
    );

    AddHttpRequestItems(*contextWrapper.Ctx(), request, PHEAD_REQUEST_ITEM, PHEAD_REQUEST_RTLOG_TOKEN_ITEM);
}

TMaybe<TPHeadResponse> RetirePheadResponse(const TScenarioHandleContext& ctx) {
    const auto pheadResponseMaybe = RetireHttpResponseProtoMaybe<NGenerativeBoltalka::Proto::TPHeadResponse>(ctx, PHEAD_RESPONSE_ITEM, PHEAD_REQUEST_ITEM);
    if (!pheadResponseMaybe) {
        LOG_INFO(ctx.Ctx.Logger()) << "Receive Nothing from phead.";
        return Nothing();
    }

    LOG_INFO(ctx.Ctx.Logger()) << "Receive from phead: " << SerializeProtoText(pheadResponseMaybe.GetRef());

    TPHeadResponse result;
    result.Scores.reserve(pheadResponseMaybe->ScoresSize());
    for (auto score : pheadResponseMaybe->GetScores()) {
        result.Scores.push_back(score);
    }
    return result;
}

} // namespace NAlice::NHollywood::NGeneralConversation
