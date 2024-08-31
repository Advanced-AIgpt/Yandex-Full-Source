#include "market_geo_support.h"

#include "types.h"

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

namespace NAlice::NHollywood::NMarket {

bool TMarketGeoSupport::IsMarketSupportedForGeoId(NGeobase::TId geoId) const
{
    NGeobase::TId countryId;
    try {
        countryId = Geobase.GetParentIdWithType(
            geoId,
            static_cast<int>(NGeobase::ERegionType::COUNTRY));
    } catch (const std::runtime_error& err) {
        LOG_ERROR(Logger)
            << "Got exception with region id " << geoId
            << ": " << ToString(err.what());
        return false;
    }
    if (IsCountryFromBKR(countryId)) {
        return true;
    }
    LOG_INFO(Logger) << "Market is not supported for region_id " << geoId;
    return false;
}

} // namespace NAlice::NHollywood::NMarket
