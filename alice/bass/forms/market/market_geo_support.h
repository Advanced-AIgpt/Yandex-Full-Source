#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/market/types.h>

namespace NBASS {

namespace NMarket {

class TMarketGeoSupport {
public:
    TMarketGeoSupport(IGlobalContext& ctx)
        : GlobalCtx(ctx)
    {
    }

    bool IsMarketSupportedForGeoId(NGeobase::TId geoId, EScenarioType scenarioType) const;
    bool IsBeruSupportedForGeoId(NGeobase::TId geoId,  EScenarioType scenarioType) const;
    bool IsMarketNativeSupportedForGeoId(NGeobase::TId geoId,  EScenarioType scenarioType) const;
    TString GetRegionName(NGeobase::TId geoId) const;

private:
    IGlobalContext& GlobalCtx;
};

} // namespace NMarket
} // namespace NBASS
