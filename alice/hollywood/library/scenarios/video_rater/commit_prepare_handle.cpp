#include "commit_prepare_handle.h"
#include "common.h"

#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/video_rater/proto/video_rater_state.pb.h>

#include <alice/library/experiments/experiments.h>
#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/resource/resource.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NVideoRater {

namespace {

const TString SAVE_RATINGS = "save_ratings";
const TString ERROR_NO_COMMIT_ARGUMENTS = "no_commit_arguments";

NJson::TJsonValue GetDataSyncRequestBody(const TVideoRaterCommitArguments& args, ui64 timestamp, const TString& timezone) {
    NJson::TJsonValue dataSyncRatings(NJson::JSON_ARRAY);
    for (const auto& rating : args.GetRating()) {
        NJson::TJsonValue dataSyncRating(NJson::JSON_MAP);
        dataSyncRating.InsertValue("kinopoisk_id", NJson::TJsonValue(rating.GetKinopoiskId()));
        dataSyncRating.InsertValue("score", NJson::TJsonValue(rating.GetScore()));
        dataSyncRating.InsertValue("text_score", NJson::TJsonValue(rating.GetTextScore()));
        dataSyncRating.InsertValue("timestamp", NJson::TJsonValue(timestamp));
        dataSyncRating.InsertValue("timezone", NJson::TJsonValue(timezone));
        dataSyncRatings.AppendValue(std::move(dataSyncRating));
    }
    NJson::TJsonValue requestBody(NJson::JSON_MAP);
    requestBody.InsertValue("value", JsonToString(dataSyncRatings));
    return requestBody;
}

} // namespace

void TVideoRaterCommitPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto& commitArgs = requestProto.GetArguments();
    if (!commitArgs.Is<TVideoRaterCommitArguments>()) {
        TCommitResponseBuilder builder;

        builder.SetError(ERROR_NO_COMMIT_ARGUMENTS, "No commit arguments present!");

        auto response = std::move(builder).BuildResponse();

        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    TVideoRaterCommitArguments ratedMovies;
    commitArgs.UnpackTo(&ratedMovies);

    TString uid = ratedMovies.GetUid();
    TString requestBody = request.ExpFlags().contains(EXP_HW_VIDEO_RATER_CLEAR_HISTORY)
        ? "{\"value\": []}"
        : JsonToString(GetDataSyncRequestBody(ratedMovies, request.ClientInfo().Epoch, request.ClientInfo().Timezone));

    LOG_INFO(ctx.Ctx.Logger()) << "Received rated movies";

    const auto datasyncRequest = PrepareDataSyncRequest(
        DATASYNC_KV_PATH,
        ctx.RequestMeta,
        TString(uid),
        ctx.Ctx.Logger(),
        SAVE_RATINGS,
        requestBody,
        NAppHostHttp::THttpRequest::Put
    );
    AddDataSyncRequestItems(ctx, datasyncRequest);
}

} // namespace NAlice::NHollywood::NVideoRater
