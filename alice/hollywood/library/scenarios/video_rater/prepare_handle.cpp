#include "prepare_handle.h"

#include "common.h"
#include "entity_search_adapter.h"

#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/experiments/experiments.h>
#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/resource/resource.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NVideoRater {

namespace {

constexpr TStringBuf DEFAULT_CANDIDATES_PATH =
    "/get?obj=lst.rec&export=json&rearr=entity_recommender_list_experiment=candidates_for_rate"sv;
constexpr TStringBuf CANDIDATES_HANDLE_AUTHORIZATION = "&client=quasar/videorater&passportid=";
const TString GET_CANDIDATE_MOVIES = "get_candidate_movies";
const TString GET_SAVED_RATINGS = "get_saved_ratings";

bool IsRelevant(const TScenarioRunRequestWrapper& request) {
    const auto initFrame = request.Input().FindSemanticFrame(INIT_FRAME);
    const auto irrelevantFrame = request.Input().FindSemanticFrame(IRRELEVANT_FRAME);
    return (initFrame || !request.IsNewSession()) && !irrelevantFrame;
}

void AddIrrelevantResponse(TScenarioHandleContext& ctx) {
    TRunResponseBuilder responseBuilder;
    responseBuilder.SetIrrelevant();
    responseBuilder.CreateResponseBodyBuilder();

    const auto response = *std::move(responseBuilder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
}

TString GetRateCandidatesPath(const TExpFlags& expFlags, const TStringBuf uid) {
    const auto rateCandidatesPath = GetExperimentValueWithPrefix(
        expFlags,
        EXP_HW_SEARCH_ENTITY_REQUEST
    ).GetOrElse(DEFAULT_CANDIDATES_PATH);

    return TString::Join(
        rateCandidatesPath,
        CANDIDATES_HANDLE_AUTHORIZATION,
        uid
    );
}

} // namespace

void TVideoRaterPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto uid = GetUid(request);

    if (!IsRelevant(request)) {
        AddIrrelevantResponse(ctx);
        return;
    }

    const auto rateCandidatesPath = GetRateCandidatesPath(request.ExpFlags(), uid);
    const auto entitySearchRequest = PrepareEntitySearchRequest(
        rateCandidatesPath,
        ctx.RequestMeta,
        ctx.Ctx.Logger(),
        GET_CANDIDATE_MOVIES
    );
    AddEntitySearchRequestItems(ctx, entitySearchRequest);

    const auto datasyncRequest = PrepareDataSyncRequest(
        DATASYNC_KV_PATH,
        ctx.RequestMeta,
        TString(uid),
        ctx.Ctx.Logger(),
        GET_SAVED_RATINGS
    );
    AddDataSyncRequestItems(ctx, datasyncRequest);
}

} // namespace NAlice::NHollywood::NVideoRater
