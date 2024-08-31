#include "vins.h"

#include <alice/hollywood/library/vins/helper.h>
#include <alice/hollywood/library/vins/hwf_state.h>
#include <alice/hollywood/library/vins/render_nlg/render_nlg.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/vins/api/vins_api/speechkit/protos/vins_response.pb.h>

namespace NAlice::NHollywoodFw::NReminders {

namespace {

constexpr TStringBuf VINS_REQUEST = "vins_request";
constexpr TStringBuf VINS_RESPONSE = "vins_response";

TStringBuf GetNlgTemplateName(const TStringBuf frameName) {
    // keep in sync with VinsProjectfile.json - https://a.yandex-team.ru/arcadia/alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json?rev=r9452987#L4510-4640
    static const auto frameNameToNlgName = THashMap<TStringBuf, TStringBuf> {
        {"personal_assistant.scenarios.alarm_reminder", "create_reminder"},
        {"personal_assistant.scenarios.create_reminder", "create_reminder"},
        {"personal_assistant.scenarios.create_reminder__ellipsis", "create_reminder"},
        {"personal_assistant.scenarios.create_reminder__cancel", "create_reminder"},

        {"personal_assistant.scenarios.list_reminders", "list_reminders"},
        {"personal_assistant.scenarios.list_reminders__scroll_next", "list_reminders"},
        {"personal_assistant.scenarios.list_reminders__scroll_reset", "list_reminders"},
        {"personal_assistant.scenarios.list_reminders__scroll_stop", "list_reminders__scroll_stop"},
    };

    return frameNameToNlgName.Value(frameName, TStringBuf());
}

} // namespace

TRemindersVinsScene::TRemindersVinsScene(const TScenario* owner)
        : TScene(owner, "vins")
{
    RegisterRenderer(&TRemindersVinsScene::RenderRun);
    RegisterRenderer(&TRemindersVinsScene::RenderApply);
}

TRetSetup TRemindersVinsScene::MainSetup(
        const TRemindersVinsSceneArgs& args,
        const TRunRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(args);

    auto requestProto = request.GetRunRequest();
    EnrichRequestFromApphostItems(request, requestProto);
    UnpackVinsState<TRemindersState>(storage, requestProto);
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

TRetMain TRemindersVinsScene::Main(
        const TRemindersVinsSceneArgs& args,
        const TRunRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);

    auto response = source.GetHttpResponseProto<NProtoVins::TVinsRunResponse>(VINS_RESPONSE);
    Y_ENSURE(response, "Vins response not found");

    auto scenarioRunResponse = std::move(*response->MutableScenarioRunResponse());
    if (auto* responseBody = GetResponseBody(scenarioRunResponse)) {
        PackVinsState<TRemindersState>(storage, *responseBody);
        if (const auto nlgTemplateName = GetNlgTemplateName(responseBody->GetSemanticFrame().GetName())) {
            RenderNlg(request, nlgTemplateName, response->GetNlgRenderData(), *responseBody);
        } else {
            LOG_WARN(request.Debug().Logger()) << "Failed to find nlg template name for semantic frame '" << responseBody->GetSemanticFrame().GetName() << "'";
        }
    }

    TRemindersVinsRunRenderArgs renderArgs;
    *renderArgs.MutableScenarioRunResponse() = std::move(scenarioRunResponse);
    return TReturnValueRender(&TRemindersVinsScene::RenderRun, renderArgs);
}

TRetSetup TRemindersVinsScene::ApplySetup(
        const TRemindersVinsSceneArgs& args,
        const TApplyRequest& request,
        const TStorage& storage) const
{
    Y_UNUSED(args);

    auto requestProto = request.GetApplyRequest();
    *requestProto.MutableArguments() = NHollywood::ReadArguments(requestProto);
    UnpackVinsState<TRemindersState>(storage, requestProto);
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

TRetContinue TRemindersVinsScene::Apply(
        const TRemindersVinsSceneArgs& args,
        const TApplyRequest& request,
        TStorage& storage,
        const TSource& source) const
{
    Y_UNUSED(args);

    auto response = source.GetHttpResponseProto<NProtoVins::TVinsApplyResponse>(VINS_RESPONSE);
    Y_ENSURE(response, "Vins response not found");

    auto scenarioApplyResponse = std::move(*response->MutableScenarioApplyResponse());
    if (auto* responseBody = GetResponseBody(scenarioApplyResponse)) {
        PackVinsState<TRemindersState>(storage, *responseBody);
        if (const auto nlgTemplateName = GetNlgTemplateName(responseBody->GetSemanticFrame().GetName())) {
            RenderNlg(request, nlgTemplateName, response->GetNlgRenderData(), *responseBody);
        } else {
            LOG_WARN(request.Debug().Logger()) << "Failed to find nlg template name for semantic frame '" << responseBody->GetSemanticFrame().GetName() << "'";
        }
    }

    TRemindersVinsApplyRenderArgs renderArgs;
    *renderArgs.MutableScenarioApplyResponse() = std::move(scenarioApplyResponse);
    return TReturnValueRender(&TRemindersVinsScene::RenderApply, renderArgs);
}

TRetResponse TRemindersVinsScene::RenderRun(const TRemindersVinsRunRenderArgs&, TRender&) {
    return TReturnValueSuccess();
}

TRetResponse TRemindersVinsScene::RenderApply(const TRemindersVinsApplyRenderArgs&, TRender&) {
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywoodFw::NReminders
