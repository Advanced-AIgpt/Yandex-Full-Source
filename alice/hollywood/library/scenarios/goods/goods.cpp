#include "goods.h"

#include <alice/hollywood/library/scenarios/goods/proto/goods.pb.h>

#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/resources/resources.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/url_builder/url_builder.h>

#include <alice/protos/analytics/goods/goods_request.pb.h>

#include <util/generic/maybe.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {
    namespace {
        constexpr TStringBuf NLG_TEMPLATE_NAME = "goods";

        constexpr TStringBuf BEST_PRICE_FRAME = "alice.goods.best_prices";
        constexpr TStringBuf BEST_PRICE_REASK_FRAME = "alice.goods.best_prices_reask";
        constexpr TStringBuf FRAMES[] = {BEST_PRICE_FRAME, BEST_PRICE_REASK_FRAME};
        constexpr TStringBuf SUGGEST_PHRASES[] = {
            "coffee_machine",
            "headphones",
            "smart_watches",
            "mixer",
            "more"
        };

        constexpr TStringBuf ONE_PRODUCT = "query_about_one_product=";
        constexpr TStringBuf MANY_PRODUCTS = "query_about_many_products=";
        constexpr TStringBuf PORNO_REQUEST = "is_porno_request=";
        constexpr TStringBuf DANGER_WORDS = "report_ydo_danger_words_all=";
        constexpr TStringBuf SHINY_POLITOTA = "shinyserp_politota=";
        constexpr TStringBuf SHINY_UNETHICAL = "shinyserp_unethical=";
        constexpr TStringBuf SHINY_DRUGS = "shinyserp_drugs=";
        constexpr TStringBuf SHINY_PORNO = "shinyserp_porno=";
        constexpr TStringBuf BANNED_REARRS[] = {DANGER_WORDS, SHINY_POLITOTA,SHINY_UNETHICAL, SHINY_DRUGS, SHINY_PORNO};

        constexpr double ONE_PRODUCT_THRESHOLD = 0.3;
        constexpr double MANY_PRODUCTS_THRESHOLD = 0.4;

        const TString ACTION_ID = "goods_button_with_uri";

        const TVector<TString> PUSH_TEXTS =  {
            "Запрос с колонки: нашла подходящие товары по запросу",
            "Запрос с колонки: смотрите, что нашла",
            "Вот, что удалось найти по запросу с колонки",
            "Кое-что нашла по запросу с колонки, смотрите",
            "Нашла, где дешевле, как и просили через колонку",
            "По запросу с колонки нашла, где дешевле"
        };

        const TString MAIN_PAGE_URL = "https://yandex.ru/products";
        const TString MAIN_PAGE_URL_PP = "viewport://?noreask=1&text=%20&viewport_id=products";

        TAnalyticsInfo::TObject CreateRequestAnalytics(const TString text, const TString url) {
            TAnalyticsInfo::TObject requestAnalytics;
            requestAnalytics.SetId("goods_request");
            requestAnalytics.SetName("goods_request");
            requestAnalytics.SetHumanReadable("Запрос товарной вертикали");
            NAnalytics::NGoodsRequest::TGoodsRequest goodsRequest;
            goodsRequest.SetUrl(url);
            goodsRequest.SetText(text);
            *requestAnalytics.MutableGoodsRequest() = goodsRequest;
            return requestAnalytics;
        }

        void AddOpenUriDirective(TResponseBodyBuilder& bodyBuilder, const TString& url) {
            TDirective directive;
            TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
            openUriDirective.SetUri(url);
            bodyBuilder.AddDirective(std::move(directive));
        }

        TLayout::TButton CreateButton(
            TResponseBodyBuilder& bodyBuilder,
            const TString& url,
            const TString& buttonText)
        {
            TFrameAction action;
            TDirective directive;
            TLayout::TButton button;
            TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
            openUriDirective.SetUri(url);
            *action.MutableDirectives()->AddList() = std::move(directive);
            bodyBuilder.AddAction(ACTION_ID, std::move(action));
            button.SetTitle(buttonText);
            button.SetActionId(ACTION_ID);
            return button;
        }

        TLayout::TButton CreateButtonAndOpenUri(
            TResponseBodyBuilder& bodyBuilder,
            IAnalyticsInfoBuilder& analyticsInfo,
            const TString& text,
            const TString& buttonText,
            bool canOpenLinkSearchViewPort)
        {
            const TString url = text.empty() ?
                (canOpenLinkSearchViewPort ? MAIN_PAGE_URL_PP : MAIN_PAGE_URL)
                : GenerateProductsSearchUri(text, canOpenLinkSearchViewPort);
            AddOpenUriDirective(bodyBuilder, url);
            const auto button = CreateButton(bodyBuilder, url, buttonText);
            analyticsInfo.AddObject(CreateRequestAnalytics(text, url));
            return button;
        }

        void AddPushMessageDirective(
            TResponseBodyBuilder& bodyBuilder,
            IAnalyticsInfoBuilder& analyticsInfo,
            const TString& text,
            const TString& title,
            const TString& body)
        {
            const TString url = GenerateProductsSearchUri(text, true);
            auto* directive = bodyBuilder.GetResponseBody().AddServerDirectives();
            NAlice::NScenarios::TPushMessageDirective* pushMessageDirective = directive->MutablePushMessageDirective();
            pushMessageDirective->SetTitle(title);
            pushMessageDirective->SetBody(body);
            pushMessageDirective->SetLink(url);
            pushMessageDirective->SetPushId("alice.goods");
            pushMessageDirective->SetPushTag("alice.goods");
            pushMessageDirective->SetThrottlePolicy("eddl-unlimitted");
            pushMessageDirective->AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);
            analyticsInfo.AddObject(CreateRequestAnalytics(text, url));
        }

        bool IsGoodsQuery(const TScenarioRunRequestWrapper& request) {
            bool isGoodsQuery = false;
            if (const auto* webSearchWizard = request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZARD)) {
                if (const auto* wizard = &webSearchWizard->GetWebSearchWizard()) {
                    const TStringBuf relev = wizard->GetRelev();
                    TStringBuf left, right;
                    if (relev.TrySplit(ONE_PRODUCT, left, right)) {
                        isGoodsQuery = isGoodsQuery || FromStringWithDefault(right.NextTok(";"), 0.0) >= ONE_PRODUCT_THRESHOLD;
                    }
                    if (relev.TrySplit(MANY_PRODUCTS, left, right)) {
                        isGoodsQuery = isGoodsQuery || FromStringWithDefault(right.NextTok(";"), 0.0) >= MANY_PRODUCTS_THRESHOLD;
                    }
                    if (relev.TrySplit(PORNO_REQUEST, left, right)) {
                        isGoodsQuery = isGoodsQuery && !FromStringWithDefault(right.NextTok(";"), 0);
                    }

                    const TStringBuf rearr = wizard->GetRearr();
                    for (const auto& flag : BANNED_REARRS) {
                        if (rearr.TrySplit(flag, left, right)) {
                            isGoodsQuery = isGoodsQuery && !FromStringWithDefault(right.NextTok(";"), 0);
                        }
                    }
                }
            }
            return isGoodsQuery;
        }

        TString GetRequestSlotValue(const TFrame& frame)
        {
            TString result;
            if (const auto slot = frame.FindSlot("request")) {
                result = slot->Value.AsString();
            }
            return result;
        }

    }

    void TGoodsHandle::Do(TScenarioHandleContext& ctx) const {

        const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
        const auto input = request.Input();

        TGoodsState state;
        bool isReask = false;
        const auto& rawState = requestProto.GetBaseRequest().GetState();
        if (rawState.Is<TGoodsState>() && !request.IsNewSession()) {
            rawState.UnpackTo(&state);
            isReask = state.GetIsReask();
            LOG_INFO(ctx.Ctx.Logger()) << "Goods state was found; IsReask is " << state.GetIsReask();
        } else if (request.IsNewSession()) {
            LOG_INFO(ctx.Ctx.Logger()) << "Goods state was dropped due to a new session";
        } else {
            LOG_INFO(ctx.Ctx.Logger()) << "Goods state was not found";
        }

        if (isReask) {
            state.SetIsReask(false);
        }

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TNlgData nlgData{ctx.Ctx.Logger(), request};
        TRunResponseBuilder builder(&nlgWrapper);

        TMaybe<TFrame> curFrame{};

        for (const auto supportedFrame : FRAMES) {
            if (input.FindSemanticFrame(supportedFrame) != nullptr) {
                curFrame = input.CreateRequestFrame(supportedFrame);
                break;
            }
        }

        auto& bodyBuilder = builder.CreateResponseBodyBuilder(curFrame.Get());

        TString text;
        auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
        analyticsInfo.SetProductScenarioName("goods");
        if (curFrame.Defined()) {
            analyticsInfo.SetIntentName(curFrame->Name());
            text = GetRequestSlotValue(curFrame.GetRef());
        }

        const bool canOpenLink = request.Interfaces().GetCanOpenLink();
        const bool canOpenLinkSearchViewPort = request.Interfaces().GetCanOpenLinkSearchViewport();
        const bool hasLogin = !GetUid(request).empty();
        const bool voiceSession = request.Interfaces().GetVoiceSession() || !input.IsTextInput();
        const bool isGoodsQuery = IsGoodsQuery(request);
        const bool isMainPageQuery = (text == nlgWrapper.RenderPhrase(NLG_TEMPLATE_NAME, "more", nlgData).Text);

        const TString buttonText = nlgWrapper.RenderPhrase(NLG_TEMPLATE_NAME, "open_uri_button_text", nlgData).Text;

        TStringBuf phraseName = "not_avail";
        TVector<TLayout::TButton> buttons;

        if (!curFrame.Defined()) {
            builder.SetIrrelevant();
        } else if ((curFrame->Name() == BEST_PRICE_REASK_FRAME) and !isReask) {
            builder.SetIrrelevant();
        } else if ((curFrame->Name() == BEST_PRICE_REASK_FRAME) &&  isReask && text.empty()) {
            builder.SetIrrelevant();
        } else if (text.empty()) {
            phraseName = "reask_product";
            state.SetIsReask(true);
            bodyBuilder.SetExpectsRequest(true);
            bodyBuilder.SetShouldListen(voiceSession);
            for (const auto& suggestPhrase : SUGGEST_PHRASES) {
                const TString suggestText =  nlgWrapper.RenderPhrase(NLG_TEMPLATE_NAME, suggestPhrase, nlgData).Text;
                bodyBuilder.AddTypeTextSuggest(suggestText);
            }
        } else if (isMainPageQuery && isReask && (canOpenLink || canOpenLinkSearchViewPort)) {
            phraseName = "main_page_request";
            const auto button = CreateButtonAndOpenUri(bodyBuilder, analyticsInfo, "", buttonText, canOpenLinkSearchViewPort);
            buttons.push_back(button);
        } else if (!isGoodsQuery) {
            builder.SetIrrelevant();
        } else if (canOpenLink || canOpenLinkSearchViewPort) {
            phraseName = "product_request";
            const auto button = CreateButtonAndOpenUri(bodyBuilder, analyticsInfo, text, buttonText, canOpenLinkSearchViewPort);
            buttons.push_back(button);
        } else if (hasLogin) {
            phraseName = "send_products_push";
            const TString randomPushBody = PUSH_TEXTS[ctx.Rng.RandomInteger(PUSH_TEXTS.size())];
            AddPushMessageDirective(bodyBuilder, analyticsInfo, text, "Алиса", randomPushBody);
        } else {
            phraseName = "not_avail";
        }


        bodyBuilder.AddRenderedTextWithButtonsAndVoice(
            NLG_TEMPLATE_NAME, phraseName, buttons, nlgData);

        auto response = std::move(builder).BuildResponse();
        auto& responseBody = *response->MutableResponseBody();
        responseBody.MutableState()->PackFrom(state);
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }

    REGISTER_SCENARIO(
        "goods",
        AddHandle<TGoodsHandle>()
            .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NGoods::NNlg::RegisterAll));
}
