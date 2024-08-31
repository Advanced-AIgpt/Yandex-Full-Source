#include "search_handler.h"

#include <alice/bass/forms/crmbot/forms.h>
#include <alice/bass/forms/crmbot/personal_data_helper.h>
#include <alice/bass/forms/crmbot/clients/report.h>
#include <alice/bass/forms/market/context.h>
#include <alice/bass/forms/market/market_exception.h>
#include <alice/bass/forms/market/market_url_builder.h>

#include <util/generic/serialized_enum.h>

namespace NBASS::NCrmbot {

void TSearchHandler::Register(THandlersMap* handlers) {
    for (EForm i : GetEnumAllValues<EForm>()) {
        handlers->emplace(ToString(i), []() { return MakeHolder<TSearchHandler>(); });
    }
}

TResultValue TSearchHandler::Do(TRequestHandler& r) {
    NMarket::TMarketContext ctx(r.Ctx());
    if (FromString<EForm>(ctx.FormName()) != EForm::Search) {
        ctx.SetResponseFormAndCopySlots(ToString(EForm::Search), {"request", "region", "intent", "url"});
    }
    TStringBuf request = ctx.GetStringSlot("request");
    if (request.empty()) {
        ctx.AddTextCardBlock("ask__slot_request");
        return TResultValue();
    }

    TStringBuf intent = ctx.GetStringSlot("intent");
    TStringBuf url = ctx.GetStringSlot("url");
    TString search_url("");
    if (url.empty()) {
        search_url = MakeSearchUrl(ctx);
        ctx.SetStringSlot("url", search_url);
    }

    if (search_url.empty()) {
        NSc::TValue data{};
        data["query"] = BuildSearchQueryText(ctx);
        ctx.AddTextCardBlock("empty_result", data);
        AddFeedbackAddon(ctx);
        return TResultValue();
    }

    NSc::TValue data{};
    data["url"] = search_url;
    switch (FromString<EIntent>(intent)) {
        case EIntent::StockCheck:
            ctx.AddTextCardBlock("stock_check", data);
            ctx.AddSuggest(TStringBuf("cant_find"));
            AddFeedbackAddon(ctx);
            break;
        case EIntent::PriceCheck:
            ctx.AddTextCardBlock("price_check", data);
            ctx.AddSuggest(TStringBuf("cant_find"));
            AddFeedbackAddon(ctx);
            break;
        case EIntent::OutOfStock:
            ctx.AddTextCardBlock("out_of_stock", data);
            ctx.AddSuggest(TStringBuf("cant_find"));
            AddFeedbackAddon(ctx);
            break;
        case EIntent::NoIntent:
        default:
            ctx.AddTextCardBlock("no_intent", data);
            if (intent.empty()) {
                NSc::TValue nobr{}, br{};
                nobr["nobr"] = true;
                br["nobr"] = false;
                ctx.AddSuggest(TStringBuf("order"), br);
                ctx.AddSuggest(TStringBuf("stock_check"), br);
                ctx.AddSuggest(TStringBuf("price_check"), nobr);
                ctx.AddSuggest(TStringBuf("warranty"), br);
                ctx.AddSuggest(TStringBuf("details"), nobr);
                ctx.AddSuggest(TStringBuf("other"), br);
            }
            break;
    };

    return TResultValue();
}

TString TSearchHandler::BuildSearchQueryText(NMarket::TMarketContext& ctx) const
{
    TStringBuf request = ctx.GetStringSlot("request");
    TStringBuf region = ctx.GetStringSlot("region");

    TStringBuilder search_query{};
    search_query << request;
    if (!region.empty()) {
        search_query << " в " << region;
    }
    return search_query;
}

TString TSearchHandler::MakeSearchUrl(NMarket::TMarketContext& ctx) const
{
    TString url(ctx.GetStringSlot("url"));
    if (!url.empty()) {
        return url;
    }

    TString search_query = BuildSearchQueryText(ctx);

    NMarket::TMarketUrlBuilder urlBuilder(ctx);
    NCrmbot::TReportClient reportClient(ctx);
    NCrmbot::TReportSearchRequestQuery reportQuery(ctx);
    auto personalData = NCrmbot::TPersonalDataHelper(ctx.Ctx());
    reportQuery.AllowRedirects(true);
    reportQuery.SetText(search_query);
    reportClient.MakeRequest(reportQuery);

    if (reportQuery.HasRedirectUrl()) {
        return urlBuilder.GetMarketUrl(NMarket::EMarketType::BLUE, reportQuery.GetRedirectUrl());
    } else if (reportQuery.HasSearchResponse()) {
        auto searchResponse = reportQuery.GetSearchResponse();
        if (searchResponse->Search().Total() > 0) {
            return urlBuilder.GetMarketSearchUrl(
                NMarket::EMarketType::BLUE,
                search_query,
                personalData.GetRegionId(),
                Nothing() /* goodState */,
                true /* redirect */
            );
        }
    }

    return TString();
}

}

