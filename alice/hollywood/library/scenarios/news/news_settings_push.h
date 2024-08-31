#pragma once

#include <alice/hollywood/library/scenarios/news/proto/apply_arguments.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/user_configs.pb.h>

namespace NAlice::NHollywood {

struct TPushBody {
    TString Title;
    TString Text;
};

inline constexpr TStringBuf ICON_ALICE = "https://yastatic.net/s3/home/apisearch/alice_icon.png";
inline constexpr TStringBuf ICON_ID = "2";
inline constexpr TStringBuf PUSH_ID = "alice_news_settings";
inline constexpr TStringBuf PUSH_URI = "yellowskin://?url=https%3A%2F%2Fyandex.ru%2Fquasar%2Faccount%2Fnews";
inline constexpr TStringBuf THROTTLE_ID = "alice_news_settings_policy";
inline constexpr TStringBuf CLIENT_IDS[] = {TStringBuf("ru.yandex.mobile"), TStringBuf("ru.yandex.searchplugin")};
inline constexpr TStringBuf MORE_INFO_NEWS = "Нажмите, чтобы открыть новость";

inline const TPushBody PUSH_BODIES[] = {
    {"Новости на выбор от Алисы", "Нажмите, чтобы выбрать свой источник новостей"},
    {"Выбирайте новости Алисы!", "Я научилась читать новости из разных источников. Нажмите, чтобы настроить."},
    {"Выбирайте новости Алисы!", "Я научилась включать новости от разных сми. Нажмите, чтобы сделать выбор"}};

bool PreparePushRequests(const TScenarioRunRequestWrapper& request, TNewsApplyArguments& applyArgs);
THttpProxyRequest CreateSupProxyRequest(const TNewsApplyArguments& applyArgs, TScenarioHandleContext& ctx);
void AddPushMessageDirective(TResponseBodyBuilder& bodyBuilder, const TStringBuf& title, const TStringBuf& text, const TStringBuf& url);

} // namespace NAlice::NHollywood::NSearch

