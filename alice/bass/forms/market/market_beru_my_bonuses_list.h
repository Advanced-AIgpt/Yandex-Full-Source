#pragma once

#include "context.h"
#include "forms.h"
#include "market_geo_support.h"

namespace NBASS {

namespace NMarket {

class TMarketBeruMyBonusesListImpl {
public:
    explicit TMarketBeruMyBonusesListImpl(TMarketContext& ctx);
    TResultValue Do();

private:
    TMarketContext& Ctx;
    EMarketBeruBonusesForm Form;
    TMarketGeoSupport GeoSupport;
};

}

}
