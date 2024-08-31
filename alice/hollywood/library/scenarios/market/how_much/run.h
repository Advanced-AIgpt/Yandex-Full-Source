#pragma once

#include "scenario.h"
#include <alice/hollywood/library/scenarios/market/common/market_geo_support.h>

namespace NAlice::NHollywood::NMarket::NHowMuch {

class TRunImpl {
public:
    TRunImpl(TMarketRunContext& ctx);
    void Do();

private:
    TMarketRunContext& Ctx;
    THowMuchScenario Scenario;
    const TMarketGeoSupport GeoSupport;

    static TString GetRequestSlotValue(const NAlice::TSemanticFrame& frame);
};

} // namespace NAlice::NHollywood::NMarket::NHowMuch
