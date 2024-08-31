#include "handles.h"

#include <alice/hollywood/library/framework/core/request.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/search/context/context.h>
#include <alice/hollywood/library/scenarios/search/nlg/register.h>
#include <alice/hollywood/library/scenarios/search/scenarios/app_navigation.h>
#include <alice/hollywood/library/scenarios/search/scenarios/base.h>
#include <alice/hollywood/library/scenarios/search/scenarios/direct.h>
#include <alice/hollywood/library/scenarios/search/scenarios/ellipsis_intents.h>
#include <alice/hollywood/library/scenarios/search/scenarios/facts.h>
#include <alice/hollywood/library/scenarios/search/scenarios/goodwin.h>
#include <alice/hollywood/library/scenarios/search/scenarios/multilang_facts.h>
#include <alice/hollywood/library/scenarios/search/scenarios/nav.h>
#include <alice/hollywood/library/scenarios/search/scenarios/navigator_intent.h>
#include <alice/hollywood/library/scenarios/search/scenarios/push_notification.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/restriction_level/restriction_level.h>
#include <alice/library/json/json.h>
#include <alice/library/scenarios/data_sources/data_sources.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

inline constexpr TStringBuf PROACTIVITY_SETTING = "/v1/personality/profile/alisa/kv/alice_proactivity";
inline constexpr TStringBuf SETTING_DISABLED = "disabled";

const TString SUMMARIZATION_APP_HOST_PARAMS = TStringBuilder{} << "summarization_" << NAppHost::APP_HOST_PARAMS_TYPE;

void LogQueryMeta(NSearch::TSearchContext& context) {
    auto& builder = context.GetAnalyticsInfoBuilder();
    builder.AddObject("tagger_query", "tagger query", context.GetTaggerQuery());
}

bool IsProactivityEnabled(const NSearch::TSearchContext& context) {
    const auto personalData = context.GetRequest().GetPersonalDataString(PROACTIVITY_SETTING);
    return !personalData || (*personalData) != SETTING_DISABLED;
}

void RunScenarios(NSearch::TSearchContext& context, NAppHost::IServiceContext& serviceCtx) {
    LogQueryMeta(context);
    if (NSearch::TMultilangFactsScenario(context).TryPrepare(serviceCtx) ||
        NSearch::CheckEllipsisIntents(context))
    {
        return;
    }

    const auto search = NSearch::GetSearchReport(context.GetRequest());
    if (!(search.Docs || search.DocsRight || search.Wizplaces ||
          search.DocsLight || search.DocsRightLight || search.WizplacesLight || search.Summarization || search.RenderrerResponse.IsDefined())) {
        context.ReturnIrrelevant();
        LOG_INFO(context.GetLogger()) << "Empty search data sources";
        return;
    }
    if (search.RenderrerResponse.IsDefined()) {
        CalculateDirectFactors(context, search);
    }
    if (context.IsPornoQuery() && context.GetRequest().ContentRestrictionLevel() == EContentSettings::children) {
        context.ReturnIrrelevant();
        LOG_INFO(context.GetLogger()) << "Porno query is disabled by content settings";
        return;
    }
    if (context.GetRequest().HasExpFlag(NExperiments::FORCE_SEARCH_GOODWIN)) {
        if (NSearch::TEntitySearchGoodwinScenario(context).Do(search)) {
            NSearch::TSearchScenario::PostProcessAnswer(context, search);
        } else {
            LOG_INFO(context.GetLogger()) << "Main intents failed to respond";
            context.ReturnIrrelevant();
        }
        return;
    }
    NSearch::TNavigatorScenario(context).Do(search);
    if (NSearch::TSearchAppNavScenario(context).FixlistAnswers(search) ||
        NSearch::TSearchNavScenario(context).Do(search) ||
        NSearch::TSearchFactsScenario(context).Do(search) ||
        NSearch::TEntitySearchGoodwinScenario(context).Do(search) ||
        NSearch::TSearchFactsScenario(context).DoObject(search) ||
        (!context.GetRequest().HasExpFlag(NExperiments::DISABLE_SERP_SUMMARIZATION) && NSearch::TSearchFactsScenario(context).DoSummarization(search)) ||
        NSearch::TSearchScenario(context).Do(search))
    {
        NSearch::TSearchScenario::PostProcessAnswer(context, search);
    } else {
        LOG_INFO(context.GetLogger()) << "Main intents failed to respond";
        context.ReturnIrrelevant(false, !context.IsPornoQuery() && IsProactivityEnabled(context));
    }
}

} // anonymous namespace

void TOpenAppsFixlistHandle::Do(TScenarioHandleContext& ctx) const {
    auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    auto context = NSearch::TSearchContext(request, builder, ctx.Ctx, ctx.RequestMeta, ctx.Rng);
    if (!NSearch::CheckEllipsisIntents(context)) {
        if (NSearch::TSearchAppNavScenario(context).FrameFixlistAnswers()) {
            context.AddResult();
        } else {
            context.ReturnIrrelevant(true);
        }
    }
    auto response = std::move(builder).BuildResponse();
    response->MutableResponseBody()->MutableState()->PackFrom(context.GetState());
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

void TSearchPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    const TMaybe<TFrame> callbackFrame = GetCallbackFrame(request.Input().GetCallback());
    if (const TMaybe<TFrame> frame = TryGetFrame("alice.push_notification", callbackFrame, request.Input()); frame.Defined()) {
        TSearchState state = request.LoadState<TSearchState>();
        TString query = state.GetPreviousQuery();
        TString link = state.GetFactoidUrl();
        if (!NSearch::PreparePushRequests(request, builder, ctx, query, link)) {
            auto context = NSearch::TSearchContext(request, builder, ctx.Ctx, ctx.RequestMeta, ctx.Rng);
            context.ReturnPushNotSuccessful();
        }
        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    auto context = NSearch::TSearchContext(request, builder, ctx.Ctx, ctx.RequestMeta, ctx.Rng);
    LOG_INFO(context.GetLogger()) << "alice.push_notification not found";
    RunScenarios(context, ctx.ServiceCtx);
    auto response = std::move(builder).BuildResponse();
    response->MutableResponseBody()->MutableState()->PackFrom(context.GetState());

    // Add search logging from the new framework
    if (ctx.NewContext != nullptr && ctx.NewContext->RunRequest != nullptr) {
        NHollywoodFw::NPrivate::TAICustomData data;
        ctx.NewContext->RunRequest->AI().BuildAnswer(response->MutableResponseBody(), data);
        Y_UNUSED(data);
    }


    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);

    for (auto const& [cardId, cardData] : context.GetBodyBuilder().GetRenderData()) {
        ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
    }

    if (requestProto.GetInput().HasCallback()) {
        return; // Skip summarization on callbacks.
    }
    if (const auto& request = context.GetSerpSummarizationAsyncRequest(); !request.Empty()) {
        if (const auto& hostPort = context.GetSerpSummarizationHostPort(); !hostPort.Empty()) {
            NJson::TJsonValue subgraphAppHostParams = ctx.AppHostParams;
            subgraphAppHostParams["srcrwr"]["SERP_SUMMARIZATION_PROXY"] = *hostPort;
            ctx.ServiceCtx.AddItem(std::move(subgraphAppHostParams), SUMMARIZATION_APP_HOST_PARAMS);

            LOG_INFO(context.GetLogger()) << "Start summarization request";
            AddHttpRequestItems(ctx, *request);
        }
    }
    if (const auto& request = context.GetSerpSummarizationRequest(); !request.Empty()) {
        LOG_INFO(context.GetLogger()) << "Summarization sync request added to context";
        ctx.ServiceCtx.AddProtobufItem(*request, "summarizer_input");
    }
}

void TSearchRenderHandle::Do(TScenarioHandleContext& ctx) const {
    auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto prepareResponseProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunResponse>(ctx.ServiceCtx, RESPONSE_ITEM);

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto context = NSearch::TSearchContext(request, builder, ctx.Ctx,
        ctx.RequestMeta, ctx.Rng, &prepareResponseProto.GetResponseBody().GetAnalyticsInfo());

    if (!NSearch::TMultilangFactsScenario(context).TryRender(ctx.ServiceCtx) &&
        !NSearch::TSearchFactsScenario(context).AddSummarizationAnswer(ctx))
    {
        return;
    }

    context.AddResult();
    auto response = std::move(builder).BuildResponse();
    if (context.GetIsUsingState()) {
        response->MutableResponseBody()->MutableState()->PackFrom(context.GetState());
    }
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}


void TSearchApplyHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (TMaybe<NScenarios::TScenarioApplyResponse> response = NSearch::TGoodwinApplyScenario(ctx.Ctx).Do(request)) {
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }
}

void TSearchCommitPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    const TSearchApplyArguments applyArgs = request.UnpackArguments<TSearchApplyArguments>();
    if (!applyArgs.GetSupRequestBody().empty()) {
        AddHttpRequestItems(ctx, NSearch::CreateSupProxyRequest(applyArgs, ctx));
        return;
    }

    if (TMaybe<NScenarios::TScenarioCommitResponse> response = NSearch::TGoodwinCommitScenario(ctx.Ctx).Do(request)) {
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }
}

void TSearchCommitRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    TCommitResponseBuilder builder;

    TMaybe<NJson::TJsonValue> supResponse = RetireHttpResponseJsonMaybe(ctx);

    if (supResponse.Defined()) {
        builder.SetSuccess();
    } else {
        builder.SetError("sup_request", "sup request failed");
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}


void TEntitySearchGoodwinCallbackHandle::Do(TScenarioHandleContext& ctx) const {
    auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (NSearch::HasEllipsisFrames(request)) {
        LOG_INFO(ctx.Ctx.Logger()) << "Found search ellipsis frames.";
        return;
    }

    if (requestProto.GetInput().HasCallback()) {
        LOG_INFO(ctx.Ctx.Logger()) << "Processing EntitySearch goodwin callback";
        NFrameFiller::TGoodwinScenarioRunHandler RunHandler(
                MakeSimpleHttpRequester(),
                [](const NFrameFiller::TSearchDocMeta& /* meta */) { return true; } // unused for callbacks
        );
        const NScenarios::TScenarioRunResponse response = NFrameFiller::Run(request, RunHandler, ctx.Ctx.Logger());
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
    }
}

REGISTER_SCENARIO("search", AddHandle<TSearchPrepareHandle>()
                            .AddHandle<TSearchRenderHandle>()
                            .AddHandle<TSearchApplyHandle>()
                            .AddHandle<TSearchCommitPrepareHandle>()
                            .AddHandle<TSearchCommitRenderHandle>()
                            .AddHandle<TEntitySearchGoodwinCallbackHandle>()
                            .SetResources<NSearch::TSearchScenarioResources>()
                            .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSearch::NNlg::RegisterAll));

REGISTER_SCENARIO("open_apps_fixlist", AddHandle<TOpenAppsFixlistHandle>()
                                       .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSearch::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
