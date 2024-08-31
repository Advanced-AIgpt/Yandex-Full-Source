#include "vins_generic_scene.h"

#include <alice/hollywood/library/vins/helper.h>
#include <alice/hollywood/library/vins/hwf_state.h>
#include <alice/hollywood/library/vins/render_nlg/render_nlg.h>
#include <alice/hollywood/library/scenarios/get_time/proto/get_time.pb.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/vins/api/vins_api/speechkit/protos/vins_response.pb.h>

namespace NAlice::NHollywoodFw::NGetTime {

namespace {
    constexpr TStringBuf VINS_REQUEST = "vins_request";
    constexpr TStringBuf VINS_RESPONSE = "vins_response";
    constexpr TStringBuf NLG_TEMPLATE_NAME = "get_time";
}

TVinsGenericScene::TVinsGenericScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_VINS_GENERIC)
{
    RegisterRenderer(&TVinsGenericScene::Render);
}

TRetSetup TVinsGenericScene::MainSetup(
    const TGetTimeVinsGenericSceneArgs& args,
    const TRunRequest& request,
    const TStorage& storage) const
{
    Y_UNUSED(args);

    auto requestProto = request.GetRunRequest();
    EnrichRequestFromApphostItems(request, requestProto);
    AddExpFlagRenderVinsNlgInHollywood(*requestProto.MutableBaseRequest());
    UnpackVinsState<TGetTimeState>(storage, requestProto);

    auto cgiParameters = BuildCgiParameters(request.GetApphostInfo().ApphosParams);
    cgiParameters.InsertUnescaped("use_vins_response_proto", "1");

    auto builder = CreateHttpRequestBuilder(request, cgiParameters);
    AddHeaders(builder, request);
    AddBody(builder, requestProto);

    TSetup setup(request);
    setup.AttachRequest(VINS_REQUEST, builder.Build().Request);
    return setup;
}

TRetMain TVinsGenericScene::Main(
    const TGetTimeVinsGenericSceneArgs& args,
    const TRunRequest& runRequest,
    TStorage& storage,
    const TSource& source) const
{
    Y_UNUSED(args);

    auto response = source.GetHttpResponseProto<NProtoVins::TVinsRunResponse>(VINS_RESPONSE);
    Y_ENSURE(response, "Vins response not found");

    auto scenarioRunResponse = std::move(*response->MutableScenarioRunResponse());
    if (auto* responseBody = GetResponseBody(scenarioRunResponse)) {
        PackVinsState<TGetTimeState>(storage, *responseBody);
        RenderNlg(runRequest, NLG_TEMPLATE_NAME, response->GetNlgRenderData(), *responseBody);
    };

    TGetTimeVinsGenericRenderArgs renderArgs;
    *renderArgs.MutableScenarioRunResponse() = std::move(scenarioRunResponse);
    return TReturnValueRender(&TVinsGenericScene::Render, renderArgs);
}

TRetResponse TVinsGenericScene::Render(const TGetTimeVinsGenericRenderArgs&, TRender&) {
    // Response is builded in Main and injected in TGetTimeScenario::Hook
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NGetTime
