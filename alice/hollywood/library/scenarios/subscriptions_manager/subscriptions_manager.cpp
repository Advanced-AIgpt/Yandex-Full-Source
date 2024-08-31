#include "subscriptions_manager.h"

#include <alice/hollywood/library/scenarios/subscriptions_manager/proto/request.pb.h>

#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/app_navigation/navigation.h>
#include <alice/library/billing/billing.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/url_builder/url_builder.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <util/generic/maybe.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf SCENARIO = "subscriptions_manager";

const TString UNEXPECTED_INTENT_ERROR_MSG = "Have not found any expected SubscriptionsManager frame";
const TString UNEXPECTED_INTENT_ERROR_TYPE = "subscriptions_manager_unexpected_intent";

const TString BILLING_PROXY = "SUBSCRIPTIONS_MANAGER_BILLING_PROXY";
constexpr TStringBuf BILLING_REQUEST_ITEM = "hw_subscriptions_billing_http_request";
constexpr TStringBuf BILLING_RESPONSE_ITEM = "hw_subscriptions_billing_http_response";

const TString PASSPORT_INFO_USER_PROXY = "SUBSCRIPTIONS_MANAGER_PASSPORT_INFO_USER_PROXY";
constexpr TStringBuf PASSPORT_INFO_USER_REQUEST_ITEM = "hw_subscriptions_passport_info_user_http_request";
constexpr TStringBuf PASSPORT_INFO_USER_RESPONSE_ITEM = "hw_subscriptions_passport_info_user_http_response";

const TString DUMMY_RTLOG_TOKEN = "dummy_rtlog_token";

TString MakeYellowskinUrl(const TString& url) {
    return TString::Join("yellowskin://?url=", CGIEscapeRet(url));
}

const TString URL_QUASAR_PURCHASES = MakeYellowskinUrl("https://yandex.ru/quasar/purchases");
const TString URL_YANDEX_PLUS = "https://plus.yandex.ru";
const TString URL_YANDEX_PLUS_MY = "https://plus.yandex.ru/my";

enum class ESubscriptionsManagerIntent { HowTo, Status, WhatWithout };

TMaybe<ESubscriptionsManagerIntent> GetIntent(const TScenarioInputWrapper& input, TRTLogger& logger, TString* intentName = nullptr) {
    ESubscriptionsManagerIntent result;
    TPtrWrapper<TSemanticFrame> frame(nullptr, "GetIntent");
    const auto trySetIntent = [&](const TStringBuf frameName, const ESubscriptionsManagerIntent intent) {
        frame = input.FindSemanticFrame(frameName);
        if (frame) {
            result = intent;
            if (intentName) {
                *intentName = frameName;
            }
            LOG_INFO(logger) << "Got " << frameName << " intent";
            return true;
        }
        return false;
    };

    if (trySetIntent("alice.subscriptions.how_to_subscribe", ESubscriptionsManagerIntent::HowTo) ||
        trySetIntent("alice.subscriptions.status", ESubscriptionsManagerIntent::Status) ||
        trySetIntent("alice.subscriptions.what_can_you_do_without_subscription", ESubscriptionsManagerIntent::WhatWithout)
    ) {
        return result;
    }

    return Nothing();
}

void MakeHttpRequest(TScenarioHandleContext& ctx, const TStringBuf name, const TString proxy, const TStringBuf path, const TStringBuf item,
                     const bool post = true, const bool auth = true) {
    auto& logger = ctx.Ctx.Logger();
    auto requestBuilder = THttpProxyRequestBuilder(path, ctx.RequestMeta, logger, proxy)
        .SetMethod(post ? NAppHostHttp::THttpRequest::Post : NAppHostHttp::THttpRequest::Get);
    if (auth) {
        requestBuilder.SetUseOAuth();
    }
    AddHttpRequestItems(ctx, requestBuilder.Build(), item);
    LOG_INFO(logger) << "Made " << name << " request";
}

TMaybe<TString> GetRawHttpResponse(TScenarioHandleContext& ctx, const TStringBuf item) {
    auto& logger = ctx.Ctx.Logger();
    if (!ctx.ServiceCtx.HasProtobufItem(item)) {
        LOG_ERR(logger) << "No " << item << " in context";
        return Nothing();
    }
    try {
        TBassRequestRTLogToken rtlogToken;
        rtlogToken.SetRTLogToken(DUMMY_RTLOG_TOKEN);
        auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, item);
        return RetireResponse(std::move(maybeResponse), rtlogToken, ctx.Ctx.Logger());
    } catch(...) {
        LOG_ERR(logger) << "Error getting " << item << ": " << CurrentExceptionMessage();
        return Nothing();
    }
}

bool GetHasPromo(TScenarioHandleContext& ctx) {
    auto& logger = ctx.Ctx.Logger();
    if (auto rawResponse = GetRawHttpResponse(ctx, BILLING_RESPONSE_ITEM)) {
        LOG_INFO(logger) << "Got billing promo response. Parsing...";
        const auto billingResp = NAlice::NBilling::ParseBillingResponse(rawResponse.GetRef());
        if (const auto error = std::get_if<NAlice::NBilling::TBillingError>(&billingResp)) {
            LOG_ERROR(logger) << "Billing response parsing error. " << error->Message() << Endl;
            return false;
        } else {
            const bool isAvailable = std::get<NAlice::NBilling::TPromoAvailability>(billingResp).IsAvailable;
            LOG_INFO(logger) << "Promo for user is " << (isAvailable ? "" : "un") << "available";
            return isAvailable;
        }
    }
    LOG_INFO(logger) << "Will act as if promo is unavailable";
    return false;
}

void AddRenderedText(TResponseBodyBuilder& bodyBuilder, const TString& phrase, const TNlgData& nlgData) {
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SCENARIO, phrase, /* buttons = */ {}, nlgData);
}

void AddOpenUriTextAndSuggest(TResponseBodyBuilder& bodyBuilder, const TString& text, const TString& uri,
                              const TScenarioApplyRequestWrapper& request, TNlgWrapper& nlgWrapper,
                              TNlgData& nlgData, TRTLogger& logger) {
    NSc::TValue info;
    auto& nav = info["nav"];
    nav["text"] = text;
    nav["url"] = uri;
    nlgData.Context["nav"] = NAlice::CreateNavBlock(info, request, /* preferApp= */ true).ToJsonValue();

    auto renderedSuggest = TBassResponseRenderer::CreateSuggest(
        nlgWrapper, SCENARIO, "opening", /* analyticsTypeAction= */ "", /* autoAction= */ true, /* data= */ {}, nlgData, &request.Interfaces());
    if (renderedSuggest.Defined()) {
        bodyBuilder.AddRenderedSuggest(std::move(renderedSuggest.GetRef()));
        AddRenderedText(bodyBuilder, "render_opening", nlgData);
        LOG_INFO(logger) << "Added suggest with directive to " << uri;
    } else {
        AddRenderedText(bodyBuilder, "error", nlgData);
        LOG_ERR(logger) << "Error rendering suggest with directive to " << uri;
    }
}

void AddSendPushDirective(TResponseBodyBuilder& bodyBuilder, const TString& title, const TString& text, const TString& url) {
    TPushDirectiveBuilder{title, text, url, "subscriptions_manager"}
        .SetThrottlePolicy("unlimited_policy")
        .BuildTo(bodyBuilder);
}

void RenderHowTo(TScenarioHandleContext& ctx, const TScenarioApplyRequestWrapper& request, TResponseBodyBuilder& bodyBuilder,
                 TNlgWrapper& nlgWrapper, TNlgData& nlgData) {
    const bool hasPromo = GetHasPromo(ctx);
    const auto& url = hasPromo ? URL_QUASAR_PURCHASES : URL_YANDEX_PLUS;

    if (request.Interfaces().GetCanOpenLink() && (!hasPromo || request.Interfaces().GetCanOpenLinkYellowskin())) {
        AddOpenUriTextAndSuggest(bodyBuilder, hasPromo ? "Настройка подписок Алисы" : "Яндекс Плюс. Настройка подписки", url,
                                 request, nlgWrapper, nlgData, ctx.Ctx.Logger());
    } else {
        AddRenderedText(bodyBuilder, TString::Join("render_how_to_subscribe__", hasPromo ? "has" : "no", "_promo"), nlgData);
        AddSendPushDirective(
            bodyBuilder,
            "Яндекс.Плюс",
            "Нажмите для активации Яндекс.Плюс",
            url
        );
    }
}

void RenderStatus(TScenarioHandleContext& ctx, const TScenarioApplyRequestWrapper& request, TResponseBodyBuilder& bodyBuilder,
                  TNlgWrapper& nlgWrapper, TNlgData& nlgData) {
    auto& logger = ctx.Ctx.Logger();
    // Just sugar, cause it is used a lot
    const auto addRenderedText = [&](const TString& phrase) {
        AddRenderedText(bodyBuilder, phrase, nlgData);
    };

    if (request.Interfaces().GetCanOpenLink()) {
        AddOpenUriTextAndSuggest(bodyBuilder, "Яндекс Плюс. Статус подписки", URL_YANDEX_PLUS_MY,
                                 request, nlgWrapper, nlgData, ctx.Ctx.Logger());
        return;
    }

    if (auto rawResponse = GetRawHttpResponse(ctx, PASSPORT_INFO_USER_RESPONSE_ITEM)) {
        LOG_INFO(logger) << "Got mediabilling passport info user response. Parsing...";
        try {
            const auto json = JsonFromString(rawResponse.GetRef());
            const auto& subscriptions = json["result"]["subscriptions"].GetArraySafe();
            if (subscriptions.empty()) {
                LOG_INFO(logger) << "User has no subscriptions";
                const bool hasPromo = GetHasPromo(ctx);
                const auto& url = hasPromo ? URL_QUASAR_PURCHASES : URL_YANDEX_PLUS_MY;
                addRenderedText(TString::Join("render_status__no_plus__", hasPromo ? "has" : "no", "_promo"));
                AddSendPushDirective(
                    bodyBuilder,
                    "Яндекс.Плюс",
                    "Нажмите для активации Яндекс.Плюс",
                    url
                );
                return;
            }
            LOG_INFO(logger) << "User has " << subscriptions.size() << " subscriptions";
            auto& nlgSubscriptions = nlgData.Context["subscriptions"];
            for (const auto& sub : subscriptions) {
                NJson::TJsonValue value;
                const TStringBuf title = sub["title"].GetStringSafe();
                // To avoid saying "подписка Новая Подписка"
                value["add_subscription_word"] = !title.Contains("одписк");
                value["title"] = title;
                value["end"] = sub["expires"].GetStringSafe();
                nlgSubscriptions.AppendValue(std::move(value));
            }
            addRenderedText("render_status__has_plus");
            AddSendPushDirective(
                bodyBuilder,
                "Яндекс.Плюс",
                "Список ваших подписок",
                URL_YANDEX_PLUS_MY
            );
        } catch (...) {
            LOG_ERR(logger) << "Error parsing mediabilling passport info user response: " << CurrentExceptionMessage();
            addRenderedText("error");
        }
    } else {
        addRenderedText("error");
    }
}

void RenderWhatWithout(const TScenarioApplyRequestWrapper& request, TResponseBodyBuilder& bodyBuilder, const TNlgData& nlgData) {
    if (request.ClientInfo().IsSmartSpeaker()) {
        const auto& interfaces = request.Interfaces();
        const bool hasScreen = interfaces.GetHasScreen() || interfaces.GetIsTvPlugged();
        AddRenderedText(bodyBuilder, TString::Join("render_what_can_you_do_without_subscription__speaker__", hasScreen ? "has" : "no", "_screen"), nlgData);
        return;
    }
    AddRenderedText(bodyBuilder, "render_what_can_you_do_without_subscription__other", nlgData);
}

template<typename T>
void AddResponse(TScenarioHandleContext& ctx, T& builder) {
    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace

// Run
void TSubscriptionsManagerRunHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    LOG_INFO(logger) << "SubscriptionsManager Run";

    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    if (const auto maybeIntent = GetIntent(request.Input(), logger); maybeIntent.Empty()) {
        builder.SetError(UNEXPECTED_INTENT_ERROR_TYPE, UNEXPECTED_INTENT_ERROR_MSG);
    } else {
        TSubscriptionsManagerRequest continueRequest;
        continueRequest.SetUid(TString{GetUid(request)});
        builder.SetContinueArguments(continueRequest);
    }

    AddResponse(ctx, builder);
}

// Continue
void TSubscriptionsManagerContinueHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto continueRequest = request.UnpackArguments<TSubscriptionsManagerRequest>();
    const auto& interfaces = request.Interfaces();
    LOG_INFO(logger) << "SubscriptionsManager Continue";

    const auto askBilling = [&]() {
        MakeHttpRequest(ctx, "billing promo", BILLING_PROXY, NAlice::NBilling::AppHostRequestPromoUrlPath(), BILLING_REQUEST_ITEM);
    };

    const auto maybeIntent = GetIntent(request.Input(), logger);
    if (maybeIntent.Empty()) {
        auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TApplyResponseBuilder builder(&nlgWrapper);
        builder.SetError(UNEXPECTED_INTENT_ERROR_TYPE, UNEXPECTED_INTENT_ERROR_MSG);
        AddResponse(ctx, builder);
        return;
    }

    switch(maybeIntent.GetRef()) {
        case ESubscriptionsManagerIntent::HowTo:
            askBilling();
            break;

        case ESubscriptionsManagerIntent::Status:
            if (!interfaces.GetCanOpenLink()) {
                askBilling();
                MakeHttpRequest(ctx, "mediabilling passport info user", PASSPORT_INFO_USER_PROXY,
                                TString::Join("?uid=", continueRequest.GetUid(), "&tld=yandex.ru"),
                                PASSPORT_INFO_USER_REQUEST_ITEM, /* post= */ false, /* auth= */ false);
            }
            break;

        case ESubscriptionsManagerIntent::WhatWithout:
            break;
    }
}

// Render
void TSubscriptionsManagerRenderHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    LOG_INFO(logger) << "SubscriptionsManager Render";

    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TApplyResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TString intentName;
    const auto maybeIntent = GetIntent(request.Input(), logger, &intentName);
    if (maybeIntent.Empty()) {
        builder.SetError(UNEXPECTED_INTENT_ERROR_TYPE, UNEXPECTED_INTENT_ERROR_MSG);
        AddResponse(ctx, builder);
        return;
    }

    Y_ASSERT(!intentName.empty());
    bodyBuilder.CreateAnalyticsInfoBuilder()
        .SetIntentName(intentName)
        .SetProductScenarioName(TString{SCENARIO});

    TNlgData nlgData{logger, request};
    switch(maybeIntent.GetRef()) {
        case ESubscriptionsManagerIntent::HowTo:
            RenderHowTo(ctx, request, bodyBuilder, nlgWrapper, nlgData);
            break;

        case ESubscriptionsManagerIntent::Status:
            RenderStatus(ctx, request, bodyBuilder, nlgWrapper, nlgData);
            break;

        case ESubscriptionsManagerIntent::WhatWithout:
            RenderWhatWithout(request, bodyBuilder, nlgData);
            break;
    }

    AddResponse(ctx, builder);
}

REGISTER_SCENARIO("subscriptions_manager",
                  AddHandle<TSubscriptionsManagerRunHandle>()
                  .AddHandle<TSubscriptionsManagerContinueHandle>()
                  .AddHandle<TSubscriptionsManagerRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NSubscriptionsManager::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
