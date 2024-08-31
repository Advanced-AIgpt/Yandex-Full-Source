#include "commit_prepare_handle.h"
#include "common.h"
#include "intents.h"

#include <alice/hollywood/library/scenarios/music/proto/music_hardcoded_arguments.pb.h>
#include <alice/hollywood/library/datasync_adapter/datasync_adapter.h>

#include <alice/library/json/json.h>

#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TString PUT_MORNING_SHOW_PUSHES = "put_morning_show_pushes";

TString PrepareDataSyncRequestBody(const TMusicHardcodedArguments& args, const ui64 timestamp) {
    NJson::TJsonValue pushes(NJson::JSON_MAP);
    pushes["pushes_sent"] = args.GetPushesSent() + 1;
    pushes["last_push_timestamp"] = timestamp;

    NJson::TJsonValue requestBody(NJson::JSON_MAP);
    requestBody["value"] = JsonToString(pushes);
    return JsonToString(requestBody);
}

} // namespace

void TCommitMusicHardcodedPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const TMusicHardcodedArguments applyArgs = request.UnpackArguments<TMusicHardcodedArguments>();
    TString uid = applyArgs.GetUid();

    if (!uid || !applyArgs.GetHasSentPush()) {
        LOG_INFO(ctx.Ctx.Logger()) << "Commit inapplicable.";
        TCommitResponseBuilder builder;
        builder.SetSuccess();
        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    const auto requestBody = PrepareDataSyncRequestBody(applyArgs, request.ClientInfo().Epoch);
    const auto datasyncRequest = PrepareDataSyncRequest(
        DATASYNC_MORNING_SHOW_PATH,
        ctx.RequestMeta,
        uid,
        ctx.Ctx.Logger(),
        PUT_MORNING_SHOW_PUSHES,
        requestBody,
        NAppHostHttp::THttpRequest::Put
    );
    AddDataSyncRequestItems(ctx, datasyncRequest);
}

} // namespace NAlice::NHollywood
