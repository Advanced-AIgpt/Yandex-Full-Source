#pragma once

#include <alice/hollywood/library/scenarios/market/common/context.h>
#include <alice/hollywood/library/scenarios/market/common/market_url_builder.h>
#include <alice/hollywood/library/scenarios/market/common/report/client.h>

#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NMarket::NHowMuch {

/* Вся специфичная для "Сколько стоит" информация, собранная в одном месте */
struct THowMuchScenario {
    THowMuchScenario(TMarketBaseContext& ctx);

    TMarketUrlBuilder UrlBuilder;
    TReportClient ReportClient;

    static constexpr TPpParam Pp = 420;
    static constexpr EClid Clid = EClid::HOW_MUCH;
    static constexpr TStringBuf ProductScenarioName = "how_much";
    static constexpr TStringBuf AnaltyicsIntent = "how_much";
    static constexpr TStringBuf NlgTemplateName = "how_much";
    static constexpr TStringBuf FrameName = "alice.market.how_much";
    static constexpr TStringBuf ScenarioName = "MarketHowMuch";

    static void SetAnalyticsInfo(NScenarios::IAnalyticsInfoBuilder& builder);
};

} // namespace NAlice::NHollywood::NMarket::NHowMuch
