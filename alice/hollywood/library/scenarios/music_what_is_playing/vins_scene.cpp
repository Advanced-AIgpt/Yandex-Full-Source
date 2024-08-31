#include "vins_scene.h"

#include <alice/hollywood/library/vins/helper.h>
#include <alice/hollywood/library/vins/hwf_state.h>
#include <alice/hollywood/library/vins/render_nlg/render_nlg.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/vins/api/vins_api/speechkit/protos/vins_response.pb.h>

namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying {

namespace {

constexpr TStringBuf VINS_REQUEST = "vins_request";
constexpr TStringBuf VINS_RESPONSE = "vins_response";

TStringBuf GetNlgTemplateName(const TStringBuf frameName) {
    // keep in sync with VinsProjectfile.json - https://a.yandex-team.ru/arcadia/alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json?rev=r9452987#L1254-1302
    static const auto frameNameToNlgName = THashMap<TStringBuf, TStringBuf> {
        {"personal_assistant.scenarios.player.what_is_playing", "music_what_is_playing"},
        {"personal_assistant.scenarios.music_what_is_playing", "music_what_is_playing"},
        {"personal_assistant.scenarios.music_what_is_playing__ellipsis", "music_what_is_playing"},
        {"personal_assistant.scenarios.music_what_is_playing__play", "music_what_is_playing__play"},
    };

    return frameNameToNlgName.Value(frameName, TStringBuf());
}

} // namespace

TMusicWhatIsPlayingVinsScene::TMusicWhatIsPlayingVinsScene(const TScenario* owner)
        : TScene(owner, SceneName)
{
    RegisterRenderer(&TMusicWhatIsPlayingVinsScene::RenderRun);
    RegisterRenderer(&TMusicWhatIsPlayingVinsScene::RenderApply);
}

TRetSetup TMusicWhatIsPlayingVinsScene::MainSetup(
        const TMusicWhatIsPlayingVinsSceneArgs& args,
        const TRunRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(args);

    auto requestProto = request.GetRunRequest();
    EnrichRequestFromApphostItems(request, requestProto);
    UnpackVinsState<TMusicWhatIsPlayingState>(storage, requestProto);
    AddExpFlagRenderVinsNlgInHollywood(*requestProto.MutableBaseRequest());

    auto cgiParameters = BuildCgiParameters(request.GetApphostInfo().ApphosParams);
    cgiParameters.InsertUnescaped("use_vins_response_proto", "1");

    auto builder = CreateHttpRequestBuilder(request, cgiParameters);
    AddHeaders(builder, request);
    AddBody(builder, requestProto);

    TSetup setup(request);
    setup.AttachRequest(VINS_REQUEST, builder.Build().Request);
    return setup;
}

TRetMain TMusicWhatIsPlayingVinsScene::Main(
        const TMusicWhatIsPlayingVinsSceneArgs& args,
        const TRunRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);

    auto response = source.GetHttpResponseProto<NProtoVins::TVinsRunResponse>(VINS_RESPONSE);
    Y_ENSURE(response, "Vins response not found");

    auto scenarioRunResponse = std::move(*response->MutableScenarioRunResponse());
    if (auto* responseBody = GetResponseBody(scenarioRunResponse)) {
        PackVinsState<TMusicWhatIsPlayingState>(storage, *responseBody);
        if (const auto nlgTemplateName = GetNlgTemplateName(responseBody->GetSemanticFrame().GetName())) {
            RenderNlg(request, nlgTemplateName, response->GetNlgRenderData(), *responseBody);
        } else {
            LOG_WARN(request.Debug().Logger()) << "Failed to find nlg template name for semantic frame '" << responseBody->GetSemanticFrame().GetName() << "'";
        }
    }

    TMusicWhatIsPlayingVinsRunRenderArgs renderArgs;
    *renderArgs.MutableScenarioRunResponse() = std::move(scenarioRunResponse);
    return TReturnValueRender(&TMusicWhatIsPlayingVinsScene::RenderRun, renderArgs);
}

TRetSetup TMusicWhatIsPlayingVinsScene::ApplySetup(
        const TMusicWhatIsPlayingVinsSceneArgs& args,
        const TApplyRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(args);

    auto requestProto = request.GetApplyRequest();
    *requestProto.MutableArguments() = NHollywood::ReadArguments(requestProto);
    UnpackVinsState<TMusicWhatIsPlayingState>(storage, requestProto);
    AddExpFlagRenderVinsNlgInHollywood(*requestProto.MutableBaseRequest());

    auto cgiParameters = BuildCgiParameters(request.GetApphostInfo().ApphosParams);
    cgiParameters.InsertUnescaped("use_vins_response_proto", "1");

    auto builder = CreateHttpRequestBuilder(request, cgiParameters);
    AddHeaders(builder, request);
    AddBody(builder, requestProto);

    TSetup setup(request);
    setup.AttachRequest(VINS_REQUEST, builder.Build().Request);
    return setup;
}

TRetContinue TMusicWhatIsPlayingVinsScene::Apply(
        const TMusicWhatIsPlayingVinsSceneArgs& args,
        const TApplyRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);

    auto response = source.GetHttpResponseProto<NProtoVins::TVinsApplyResponse>(VINS_RESPONSE);
    Y_ENSURE(response, "Vins response not found");

    auto scenarioApplyResponse = std::move(*response->MutableScenarioApplyResponse());
    if (auto* responseBody = GetResponseBody(scenarioApplyResponse)) {
        PackVinsState<TMusicWhatIsPlayingState>(storage, *responseBody);
        if (const auto nlgTemplateName = GetNlgTemplateName(responseBody->GetSemanticFrame().GetName())) {
            RenderNlg(request, nlgTemplateName, response->GetNlgRenderData(), *responseBody);
        } else {
            LOG_WARN(request.Debug().Logger()) << "Failed to find nlg template name for semantic frame '" << responseBody->GetSemanticFrame().GetName() << "'";
        }
    }

    TMusicWhatIsPlayingVinsApplyRenderArgs renderArgs;
    *renderArgs.MutableScenarioApplyResponse() = std::move(scenarioApplyResponse);
    return TReturnValueRender(&TMusicWhatIsPlayingVinsScene::RenderApply, renderArgs);
}

TRetResponse TMusicWhatIsPlayingVinsScene::RenderRun(const TMusicWhatIsPlayingVinsRunRenderArgs&, TRender&) {
    return TReturnValueSuccess();
}

TRetResponse TMusicWhatIsPlayingVinsScene::RenderApply(const TMusicWhatIsPlayingVinsApplyRenderArgs&, TRender&) {
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying
