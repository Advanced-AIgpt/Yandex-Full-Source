#include "vins_scene.h"

#include <alice/hollywood/library/scenarios/vins/div_cards/show_route.h>
#include <alice/hollywood/library/scenarios/vins/div_cards/show_traffic.h>

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/vins/helper.h>

#include <alice/vins/api/vins_api/speechkit/protos/vins_response.pb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <alice/protos/api/renderer/api.pb.h>

namespace NAlice::NHollywoodFw::NVins {

inline constexpr TStringBuf VINS_REQUEST = "vins_request";

using namespace NScenarios;
using namespace NHollywood;

TVinsScene::TVinsScene(const TScenario* owner)
        : TScene(owner, "vins")
{
    RegisterRenderer(&TVinsScene::RenderRun);
    RegisterRenderer(&TVinsScene::RenderScreenDevice);
    RegisterRenderer(&TVinsScene::RenderApply);

    TDivCardProcessor::Instance().Register<TProcessorShowRoute>();
    TDivCardProcessor::Instance().Register<TProcessorShowTraffic>();
}

TRetSetup TVinsScene::MainSetup(
        const TVinsSceneArgs& args,
        const TRunRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(storage);

    TScenarioRunRequest requestProto = request.GetRunRequest();
    EnrichRequestFromApphostItems(request, requestProto);

    auto cgiParameters = BuildCgiParameters(request.GetApphostInfo().ApphosParams);
    cgiParameters.InsertUnescaped("use_vins_response_proto", "1");
    if (args.GetUseScreenDeviceRender()) {
        AddExpFlag("pass_through_bass_response_to_hw", *requestProto.MutableBaseRequest());
    }

    auto builder = CreateHttpRequestBuilder(request, cgiParameters);
    AddHeaders(builder, request);
    AddBody(builder, requestProto);

    TSetup setup(request);
    setup.AttachRequest(VINS_REQUEST, builder.Build().Request);
    return setup;
}

TRetMain TVinsScene::Main(
        const TVinsSceneArgs& args,
        const TRunRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(request);
    Y_UNUSED(storage);

    auto vinsResponse = source.GetHttpResponseProto<NProtoVins::TVinsRunResponse>().GetRef();
    LOG_DEBUG(request.Debug().Logger()) << "Got response " << vinsResponse.GetTypeName() << ": " << vinsResponse;

    if (args.GetUseScreenDeviceRender()) {
        TDivCardResult res;
        auto rc = TDivCardProcessor::Instance().Process(vinsResponse, res);
        switch (rc) {
            case TDivCardProcessor::EProcessorRet::Unknown: {
                LOG_INFO(request.Debug().Logger()) << "No suitable screen device renderer for vins";
                break;
            }
            case TDivCardProcessor::EProcessorRet::Success: {
                NRenderer::TDivRenderData divCard;
                divCard.SetCardId(res.DivCardName);
                *divCard.MutableScenarioData() = std::move(res.ScenarioRenderCard);
                TScreenDeviceRender renderArgs;
                renderArgs.MutableResponse()->Swap(vinsResponse.MutableScenarioRunResponse());
                renderArgs.SetDivCardName(res.DivCardName);
                auto render = TReturnValueRender(&TVinsScene::RenderScreenDevice, renderArgs);
                return render.AddDivRender(std::move(divCard));
            }
        }
    }

    const auto& response = vinsResponse.GetScenarioRunResponse();
    switch (response.GetResponseCase()) {
        case TScenarioRunResponse::kApplyArguments: {
            return TReturnValueApplyUnpacked(response.GetApplyArguments(), response.GetFeatures());
        }
        case TScenarioRunResponse::kCommitCandidate: {
            return TReturnValueCommit(&TVinsScene::RenderRun, response, response.GetCommitCandidate().GetArguments(), response.GetFeatures());
        }
        default:
            break;
    }
    return TReturnValueRender(&TVinsScene::RenderRun, response);
}

TRetSetup TVinsScene::ApplySetup(
        const TVinsSceneArgs& args,
        const TApplyRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(args);
    Y_UNUSED(storage);

    TScenarioApplyRequest requestProto = request.GetApplyRequest();
    *requestProto.MutableArguments() = ReadArguments(requestProto);

    auto cgiParameters = BuildCgiParameters(request.GetApphostInfo().ApphosParams);
    auto builder = CreateHttpRequestBuilder(request, cgiParameters);
    AddHeaders(builder, request);
    AddBody(builder, requestProto);

    TSetup setup(request);
    setup.AttachRequest(VINS_REQUEST, builder.Build().Request);
    return setup;
}

TRetContinue TVinsScene::Apply(
        const TVinsSceneArgs& args,
        const TApplyRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);
    Y_UNUSED(storage);

    auto response = source.GetHttpResponseProto<TScenarioApplyResponse>().GetRef();
    LOG_DEBUG(request.Debug().Logger()) << "Got response " << response.GetTypeName() << ": " << response;
    return TReturnValueRender(&TVinsScene::RenderApply, response);
}

TRetSetup TVinsScene::CommitSetup(
        const TVinsSceneArgs& args,
        const TCommitRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(args);
    Y_UNUSED(storage);

    TScenarioApplyRequest requestProto = request.GetCommitRequest();
    *requestProto.MutableArguments() = ReadArguments(requestProto);

    auto cgiParameters = BuildCgiParameters(request.GetApphostInfo().ApphosParams);
    auto builder = CreateHttpRequestBuilder(request, cgiParameters);
    AddHeaders(builder, request);
    AddBody(builder, requestProto);

    TSetup setup(request);
    setup.AttachRequest(VINS_REQUEST, builder.Build().Request);
    return setup;
}

TRetCommit TVinsScene::Commit(
        const TVinsSceneArgs& args,
        const TCommitRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);
    Y_UNUSED(storage);

    auto response = source.GetHttpResponseProto<TScenarioCommitResponse>().GetRef();
    LOG_DEBUG(request.Debug().Logger()) << "Got response " << response.GetTypeName() << ": " << response;
    switch (response.GetResponseCase()) {
        case TScenarioCommitResponse::kError: {
            TError error(TError::EErrorDefinition::Exception);
            error.Details() << response.GetError().GetMessage();
            return error;
        }
        case TScenarioCommitResponse::kSuccess:
            return TReturnValueSuccess();
        default:
            break;
    }
    HW_ERROR("Undefined status of vins response");
}

TRetResponse TVinsScene::RenderRun(
        const TScenarioRunResponse& args,
        TRender& render)
{
    Y_UNUSED(args);
    Y_UNUSED(render);
    return TReturnValueSuccess();
}

TRetResponse TVinsScene::RenderScreenDevice(
        const TScreenDeviceRender& args,
        TRender& render)
{
    NAlice::NScenarios::TShowViewDirective showViewDirective;
    showViewDirective.SetName("show_view");
    showViewDirective.SetDoNotShowCloseButton(true);
    showViewDirective.SetInactivityTimeout(TShowViewDirective_EInactivityTimeout_Infinity);
    *showViewDirective.MutableLayer()->MutableDialog() = NScenarios::TShowViewDirective::TLayer::TDialogLayer{};
    showViewDirective.SetCardId(args.GetDivCardName());
    render.Directives().AddShowViewDirective(std::move(showViewDirective));

    NScenarios::TTtsPlayPlaceholderDirective ttsDirective;
    ttsDirective.SetName("tts_play_placeholder");
    render.Directives().AddTtsPlayPlaceholderDirective(std::move(ttsDirective));
    render.SetShouldListen(true);

    return TReturnValueSuccess();
}

TRetResponse TVinsScene::RenderApply(
        const TScenarioApplyResponse& args,
        TRender& render)
{
    Y_UNUSED(args);
    Y_UNUSED(render);
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywoodFw::NVins
