#include "news_settings_push.h"

#include <alice/hollywood/library/scenarios/news/proto/apply_arguments.pb.h>

#include <alice/megamind/protos/common/app_type.pb.h>

#include <util/system/env.h>

namespace NAlice::NHollywood {
namespace {

bool AppendSupPush(
    TNewsApplyArguments& applyArgs, const TScenarioRunRequestWrapper& request,
    const TStringBuf title, const TStringBuf link, const TStringBuf bodyText, const TStringBuf pushId)
{
    NSc::TValue body;
    TStringBuilder receiver;
    TStringBuf uid = GetUid(request);
    if (uid.empty()) {
        return false;
    }
    receiver << "tag:uid=='" << uid << "' AND (";
    bool first = true;
    for (const auto& client: CLIENT_IDS) {
        if (!first) {
            receiver << " OR ";
        }
        receiver << "app_id=='" << client << "'";
        first = false;
    }
    receiver << ")";
    body["receiver"].SetArray().Push().SetString(receiver);
    body["ttl"].SetIntNumber(360);
    NSc::TValue& notification = body["notification"];

    notification["body"].SetString(bodyText);
    notification["link"].SetString(link);
    notification["title"].SetString(title);
    notification["icon"].SetString(ICON_ALICE);
    notification["iconId"].SetString(ICON_ID);
    body["data"]["tag"].SetString(pushId);
    body["data"]["push_id"].SetString(pushId);
    body["data"]["transit_id"].SetString(request.Proto().GetBaseRequest().GetRequestId());
    body["project"].SetString("bass");
    body["max_expected_receivers"].SetIntNumber(10);
    NSc::TValue& throttlePolicy = body["throttle_policies"];
    throttlePolicy["install_id"].SetString(THROTTLE_ID);
    throttlePolicy["device_id"].SetString(THROTTLE_ID);
    applyArgs.SetSupRequestBody(body.ToJson());
    return true;
}

} // anonymous namespace

bool PreparePushRequests(const TScenarioRunRequestWrapper& request, TNewsApplyArguments& applyArgs)
{
    // TODO: прокидывать контекст для случайного выбора тела пуша
    const auto& pushBody = PUSH_BODIES[1];
    return AppendSupPush(applyArgs, request, pushBody.Title, PUSH_URI, pushBody.Text, PUSH_ID);
}

THttpProxyRequest CreateSupProxyRequest(const TNewsApplyArguments& applyArgs, TScenarioHandleContext& ctx) {
    const TString& body = applyArgs.GetSupRequestBody();
    THttpHeaders headers;
    headers.AddHeader("Content-Type", "application/json;charset=UTF-8");
    headers.AddHeader("Authorization", TStringBuilder() << "OAuth " << GetEnv("SUP_TOKEN"));
    return PrepareHttpRequest("/pushes", ctx.RequestMeta, ctx.Ctx.Logger(),
        "sup_request", body, NAppHostHttp::THttpRequest::Post, headers);
}

void AddPushMessageDirective(
        TResponseBodyBuilder& bodyBuilder,
        const TStringBuf& title,
        const TStringBuf& text,
        const TStringBuf& url) 
    {
    auto* directive = bodyBuilder.GetResponseBody().AddServerDirectives();
    NAlice::NScenarios::TPushMessageDirective* pushMessageDirective = directive->MutablePushMessageDirective();

    // TODO: сделать случайный выбор тела пуша
    pushMessageDirective->SetTitle(title.Data());
    pushMessageDirective->SetBody(text.Data());
    pushMessageDirective->SetLink(url.Data());
    pushMessageDirective->SetPushId(PUSH_ID.Data());
    pushMessageDirective->SetPushTag(PUSH_ID.Data());
    pushMessageDirective->SetThrottlePolicy(THROTTLE_ID.Data());
    pushMessageDirective->AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);
}

} // namespace NAlice::NHollywood::NSearch

