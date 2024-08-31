#include "scenario.h"

namespace NAlice::NHollywood::NMarket::NHowMuch {

THowMuchScenario::THowMuchScenario(TMarketBaseContext& ctx)
    : UrlBuilder(CreateMarketUrlBuilder(Clid, ctx.RequestWrapper()))
    , ReportClient(CreateReportClient(Clid, Pp, ctx.RequestWrapper()))
{}

void THowMuchScenario::SetAnalyticsInfo(NScenarios::IAnalyticsInfoBuilder& builder)
{
    builder.SetProductScenarioName(ToString(ProductScenarioName));
    builder.SetIntentName(ToString(AnaltyicsIntent));
}

} // namespace NAlice::NHollywood::NMarket::NHowMuch
