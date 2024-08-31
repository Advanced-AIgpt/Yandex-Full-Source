#pragma once

#include <alice/library/logger/logger.h>
#include <library/cpp/geobase/lookup.hpp>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NMarket {

class TMarketGeoSupport {
public:
    TMarketGeoSupport(const NGeobase::TLookup& geobase, TRTLogger& logger)
        : Geobase(geobase)
        , Logger(logger)
    {
    }

    bool IsMarketSupportedForGeoId(NGeobase::TId geoId) const;

private:
    const NGeobase::TLookup& Geobase;
    TRTLogger& Logger;
};

} // namespace NAlice::NHollywood::NMarket
