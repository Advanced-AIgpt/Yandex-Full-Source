#include "run_search_content_post_handle.h"
#include "common.h"

#include <alice/library/experiments/flags.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/scenarios/music/common.h>

#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf DATA = "data";
constexpr TStringBuf ERROR = "error";
constexpr TStringBuf FEATURES_DATA = "features_data";
constexpr TStringBuf IS_PLAYER_COMMAND = "is_player_command";
constexpr TStringBuf SEARCH_RESULT = "search_result";
constexpr TStringBuf SEARCH_TEXT = "search_text";

const THashSet<TStringBuf> IRRELEVANT_ERROR_CODES = {
    TStringBuf("irrelevant_player_command"),
    TStringBuf("nothing_is_playing")
};

// TODO(sparkle): maybe don't copy-paste?..
std::unique_ptr<NScenarios::TScenarioRunResponse> RenderResponseBody(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const NJson::TJsonValue& bassState)
{
    auto& logger = ctx.Ctx.Logger();

    THwFrameworkRunResponseBuilder builder{ctx, &nlg};
    TBassResponseRenderer bassRenderer(request, request.Input(), builder, logger,
                                       /* suggestAutoAction= */ false);
    bassRenderer.Render(TEMPLATE_MUSIC_PLAY, "render_result", bassState);

    if (const auto* errorBlock = FindBlock(bassState, ERROR)) {
        LOG_INFO(logger) << "Found the error block";
        const auto& code = (*errorBlock)[DATA]["code"].GetString();
        if (IRRELEVANT_ERROR_CODES.contains(code)) {
            LOG_INFO(logger) << "Found the " << code << " error, setting the Irrelevant flag";
            builder.SetIrrelevant();
        }
    }

    if (const auto& featuresData = bassState[FEATURES_DATA]; featuresData.IsDefined()) {
        builder.FillMusicFeatures(featuresData[SEARCH_TEXT].GetString(),
                                   featuresData[SEARCH_RESULT],
                                   featuresData[IS_PLAYER_COMMAND].GetBoolean());
    }

    return std::move(builder).BuildResponse();
}

std::unique_ptr<NScenarios::TScenarioRunResponse> RunSearchContentPostDoImpl(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const NJson::TJsonValue& bassResponse)
{
    auto& logger = ctx.Ctx.Logger();

    const TStringBuf continuationName = bassResponse["ObjectTypeName"].GetString();
    LOG_DEBUG(logger) << "Unpacking the continuation state";
    const TString& bassStateStr = bassResponse["State"].GetStringSafe();
    const NJson::TJsonValue bassState = JsonFromString(bassStateStr);

    if (continuationName == "TCompletedContinuation") {
        return RenderResponseBody(ctx, request, nlg, bassState);
    }
    Y_ENSURE(continuationName == "TMusicContinuation");
    Y_ENSURE(!bassResponse["IsFinished"].GetBoolean());

    const auto frameProto = request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME);
    Y_ENSURE(frameProto);
    const auto frame = TFrame::FromProto(*frameProto);

    const auto* userInfo = GetUserInfoProto(request);
    Y_ENSURE(userInfo);

    NJson::TJsonValue musicArguments;
    musicArguments["bass_scenario_state"] = bassStateStr;

    FillFairyTaleInfo(musicArguments, bassState, request);

    bool isSpecialAlbum = false;
    FillMusicSearchResult(
        logger,
        musicArguments,
        bassState,
        frame,
        userInfo->GetUid(),
        /* supportsPlaylists= */ true,
        &isSpecialAlbum
    );

    auto flowType = TMusicArguments_EExecutionFlowType_MusicSdkSubgraph;
    // FIXME(sparkle): support special albums at MusicSDK https://st.yandex-team.ru/HOLLYWOOD-634
    if (isSpecialAlbum) {
        flowType = TMusicArguments_EExecutionFlowType_BassDefault;
    }
    musicArguments["execution_flow_type"] = TMusicArguments_EExecutionFlowType_Name(flowType);

    // TODO(sparkle): to common methods
    musicArguments["account_status"]["uid"] = userInfo->GetUid();
    musicArguments["account_status"]["has_plus"] = userInfo->GetHasYandexPlus();
    musicArguments["account_status"]["has_music_subscription"] = userInfo->GetHasMusicSubscription();

    const auto* environmentStateProto = GetEnvironmentStateProto(request);
    if (environmentStateProto) {
        FillEnvironmentState(musicArguments, *environmentStateProto);
    }

    LOG_DEBUG(logger) << "Pre JsonToProto MusicArguments: " << JsonToString(musicArguments);
    auto continueArgs = JsonToProto<TMusicArguments>(musicArguments, /* validateUtf8= */ true,
                                                     /* ignoreUnknownFields= */ true);
    LOG_DEBUG(logger) << "Post JsonToProto MusicArguments: " << JsonStringFromProto(continueArgs);

    // Setup user location
    if (const auto* userLocationPtr = request.GetDataSource(NAlice::EDataSourceType::USER_LOCATION)) {
        continueArgs.MutableUserLocation()->CopyFrom(userLocationPtr->GetUserLocation());
    }

    if (const auto* envStatePtr = request.GetDataSource(NAlice::EDataSourceType::ENVIRONMENT_STATE)) {
        continueArgs.MutableEnvironmentState()->CopyFrom(envStatePtr->GetEnvironmentState());
    }

    THwFrameworkRunResponseBuilder response{ctx, &nlg};
    response.SetFeaturesIntent(frame.Name());
    if (const auto& featuresData = bassState[FEATURES_DATA]; featuresData.IsDefined()) {
        response.FillMusicFeatures(featuresData[SEARCH_TEXT].GetString(),
                                   featuresData[SEARCH_RESULT],
                                   featuresData[IS_PLAYER_COMMAND].GetBoolean());
    }
    response.SetContinueArgumentsOldFlow(continueArgs);
    return std::move(response).BuildResponse();
}

} // namespace

void TMusicSdkRunSearchContentPostHandle::Do(TScenarioHandleContext& ctx) const {
    const NJson::TJsonValue bassResponse = RetireBassRequest(ctx);

    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);

    const auto response = RunSearchContentPostDoImpl(ctx, request, nlg, bassResponse);
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
