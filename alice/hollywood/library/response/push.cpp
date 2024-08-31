#include "push.h"

#include <alice/hollywood/library/framework/core/codegen/gen_server_directives.pb.h>
#include <alice/hollywood/library/framework/core/render.h>
#include <alice/hollywood/library/framework/core/request.h>

namespace NAlice::NHollywood {

// All necessary fields are filled in constructor
TPushDirectiveBuilder::TPushDirectiveBuilder(const TString& title,
                                             const TString& text,
                                             const TString& url,
                                             const TString& tag)
{
    auto& settings = *Push.MutableSettings();
    settings.SetTitle(title);
    settings.SetText(text);
    settings.SetLink(url);
    settings.SetTtlSeconds(DEFAULT_PUSH_TTL);

    Push.SetPushId(tag);
    Push.SetPushTag(tag);

    Push.MutablePushMessage()->AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);

    SetAnalyticsAction("send_push_" + tag, "send push " + tag, "Отправляется ссылка в приложение");
}

TPushDirectiveBuilder& TPushDirectiveBuilder::SetTtlSeconds(ui64 ttl) {
    Push.MutableSettings()->SetTtlSeconds(ttl);
    return *this;
}

// For when it is not equal to tag
TPushDirectiveBuilder& TPushDirectiveBuilder::SetPushId(const TString& id) {
    Push.SetPushId(id);
    return *this;
}

// Doc: https://doc.yandex-team.ru/sup-api/concepts/push-create/index.html
// Values: https://sup.yandex.net/throttle-policies
TPushDirectiveBuilder& TPushDirectiveBuilder::SetThrottlePolicy(const TString& policy) {
    Push.MutablePushMessage()->SetThrottlePolicy(policy);
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::SetAppTypes(const TVector<NAlice::EAppType>& appTypes) {
    auto& pushMessage = *Push.MutablePushMessage();
    pushMessage.ClearAppTypes();
    for (const auto& appType : appTypes) {
        pushMessage.AddAppTypes(appType);
    }
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::SetCardImageUrl(const TString& url) {
    Push.MutablePersonalCard()->SetImageUrl(url);
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::SetCardDateRange(const TString& from, const TString& to) {
    auto& card = *Push.MutablePersonalCard();
    card.SetDateFrom(from);
    card.SetDateTo(to);
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::RemoveExistingCards() {
    Push.SetRemoveExistingCards(true);
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::OverridePushSettings(const NScenarios::TSendPushDirective::TCommon& settings) {
    Push.MutablePushMessage()->MutableSettings()->CopyFrom(settings);
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::OverrideCardSettings(const NScenarios::TSendPushDirective::TCommon& settings) {
    Push.MutablePersonalCard()->MutableSettings()->CopyFrom(settings);
    return *this;
}

TPushDirectiveBuilder& TPushDirectiveBuilder::SetAnalyticsAction(TString id, TString name, TString description) {
    AnalyticsAction.SetId(std::move(id));
    AnalyticsAction.SetName(std::move(name));
    AnalyticsAction.SetHumanReadable(std::move(description));
    return *this;
}

void TPushDirectiveBuilder::BuildTo(TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TServerDirective directive;
    *directive.MutableSendPushDirective() = std::move(Push);
    bodyBuilder.AddServerDirective(std::move(directive));
    bodyBuilder.GetOrCreateAnalyticsInfoBuilder().AddAction(std::move(AnalyticsAction));
}

void TPushDirectiveBuilder::BuildTo(NHollywoodFw::TRender& render) {
    render.ServerDirectives().AddSendPushDirective(std::move(Push));
    render.GetRequest().AI().AddAction(std::move(AnalyticsAction));
}

} // namespace NAlice::NHollywood
