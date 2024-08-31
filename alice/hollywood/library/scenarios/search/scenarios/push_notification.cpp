#include "base.h"
#include "ellipsis_intents.h"
#include "push_notification.h"

#include <alice/hollywood/library/response/push.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/url_builder/url_builder.h>
#include <library/cpp/http/simple/http_client.h>
#include <util/system/env.h>

namespace NAlice::NHollywood::NSearch {
namespace {
static const TCgiParameters SEARCH_QUERY_CGI{TStringBuf("query_source=smart_speaker&source=smart_speaker")};

void AddSearchPushMessageDirective(const TString& title, const TString& link, const TString& bodyText, TResponseBodyBuilder& bodyBuilder) {
    TPushDirectiveBuilder{title, bodyText, link, TString{PUSH_ID}}
        .SetThrottlePolicy(TString{THROTTLE_ID})
        .SetTtlSeconds(900)
        .BuildTo(bodyBuilder);
}

} // anonymous namespace

bool PreparePushRequests(
    const TScenarioRunRequestWrapper& request, TRunResponseBuilder& builder,
    TScenarioHandleContext& ctx, const TString& query, const TString& customUri)
{
    if (query.empty()) {
        LOG_ERR(ctx.Ctx.Logger()) << "Empty query, not sending push.";
        return false;
    }
    TUserLocation userLocation = GetUserLocation(request);
    TString pushUri, titleText;
    if (request.HasExpFlag(NExperiments::PUSH_PATH_SITE)) {
        pushUri = customUri;
        titleText = TStringBuilder() << SITE_TITLE_TEXT << GetHost(customUri);
    } else {
        pushUri = NAlice::GenerateSearchUri(request.ClientInfo(), userLocation,
                                            request.ContentRestrictionLevel(), query,
                                            request.Interfaces().GetCanOpenLinkSearchViewport(), SEARCH_QUERY_CGI, true);
        titleText = SERP_TITLE_TEXT;
    }

    const TString bodyText = TString::Join("Нажмите, чтобы найти \"", query, "\"");

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    AddSearchPushMessageDirective(titleText, pushUri, bodyText, bodyBuilder);

    TNlgData nlgData{ctx.Ctx.Logger(), request};
    if (!request.HasExpFlag(NExperiments::DISABLE_PUSH_ANSWER_PP_REMINDER)) {
        nlgData.Context["attentions"]["search__push_sent_pp_reminder"] = true;
    } else {
        nlgData.Context["attentions"]["search__push_sent"] = true;
    }
    bodyBuilder.AddRenderedTextWithButtonsAndVoice("search", "render_result", /* buttons = */ {}, nlgData);
    if (request.HasExpFlag(NExperiments::HANDOFF_LISTEN_AFTER_PUSH)) {
        bodyBuilder.SetShouldListen(true);
    }
    return true;
}

THttpProxyRequest CreateSupProxyRequest(const TSearchApplyArguments& applyArgs, TScenarioHandleContext& ctx) {
    const TString& body = applyArgs.GetSupRequestBody();
    static const TString supHeader = TStringBuilder() << "OAuth " << GetEnv("SUP_TOKEN");

    THttpHeaders headers;
    headers.AddHeader("Content-Type", "application/json;charset=UTF-8");
    headers.AddHeader("Authorization", supHeader);

    return PrepareHttpRequest("/pushes", ctx.RequestMeta, ctx.Ctx.Logger(),
        "sup_request", body, NAppHostHttp::THttpRequest::Post, headers);
}

} // namespace NAlice::NHollywood::NSearch
