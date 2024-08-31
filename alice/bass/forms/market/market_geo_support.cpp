#include "market_geo_support.h"

#include "types.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <kernel/geodb/countries.h>

namespace {
    using namespace NGeoDB;
    static constexpr NGeobase::TId BKR_IDS[] = {BELARUS_ID, KAZAKHSTAN_ID, RUSSIA_ID};

    inline bool IsCountryFromBKR(const NGeobase::TId id) noexcept {
        for (const auto country : BKR_IDS) {
            if (country == id) {
                return true;
            }
        }

        return false;
    }
}

namespace NBASS {

namespace NMarket {

bool TMarketGeoSupport::IsMarketSupportedForGeoId(NGeobase::TId geoId, EScenarioType scenarioType) const
{
    TRequestedGeo userGeo(GlobalCtx, geoId);
    NGeobase::TId countryId = userGeo.GetParentIdByType(NGeobase::ERegionType::COUNTRY);
    if (IsCountryFromBKR(countryId)) {
        return true;
    }
    LOG(INFO) << "User located in " << GetRegionName(countryId) << "(" << countryId << ") "
              << "where " << ToString(scenarioType) <<" is not presented" << Endl;
    return false;
}

bool TMarketGeoSupport::IsBeruSupportedForGeoId(NGeobase::TId geoId, EScenarioType scenarioType) const
{
    TRequestedGeo userGeo(GlobalCtx, geoId);
    NGeobase::TId countryId = userGeo.GetParentIdByType(NGeobase::ERegionType::COUNTRY);
    if (countryId == NGeoDB::RUSSIA_ID) {
        return true;
    }
    LOG(INFO) << "User located in " << GetRegionName(countryId) << "(" << countryId << ") "
              << "where " << ToString(scenarioType) <<" is not presented" << Endl;
    return false;
}

bool TMarketGeoSupport::IsMarketNativeSupportedForGeoId(NGeobase::TId geoId, EScenarioType scenarioType) const {
    return IsBeruSupportedForGeoId(geoId, scenarioType);
}

TString TMarketGeoSupport::GetRegionName(NGeobase::TId geoId) const {
    TString name, namePrepcase;
    TRequestedGeo userGeo(GlobalCtx, geoId);
    NGeobase::TId countryId = userGeo.GetParentIdByType(NGeobase::ERegionType::COUNTRY);
    NAlice::GeoIdToNames(GlobalCtx.GeobaseLookup(), countryId, TStringBuf("ru"), &name, &namePrepcase);
    return name;
}

} // namespace NMarket

} // namespace NBASS
