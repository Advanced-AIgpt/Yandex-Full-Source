#include "special_location.h"

#include <alice/bass/forms/geocoder.h>
#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/globalctx/globalctx.h>

namespace NBASS {

NGeobase::TId TSpecialLocation::GetGeo(TContext& ctx, TResultValue* result) const {
    NGeobase::TId geoId = NGeobase::UNKNOWN_REGION;

    switch (Value) {
        case EType::CURRENT_LOCALITY:
        case EType::NEAR_ME:
        case EType::NEARBY: // <- temporary solutinon for nearby
        case EType::NEAREST:
            geoId = ctx.UserRegion();
            if (!NAlice::IsValidId(geoId)) {
                // this is a stub, error should be more specific
                *result = TError(TError::EType::NOGEOFOUND, "No geo found for user neighborhood");
            }
            break;

        case EType::CURRENT_COUNTRY: {
            auto userId = ctx.UserRegion();
            if (!NAlice::IsValidId(userId)) {
                // this is a stub, error should be more specific
                *result = TError(TError::EType::NOGEOFOUND, "No geo found for user country");
            }
            const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
            auto countryTypeId = static_cast<int>(NGeobase::ERegionType::COUNTRY);
            geoId = geobase.GetParentIdWithType(userId, countryTypeId);
            if (!NAlice::IsValidId(geoId)) {
                // this is a stub, error should be more specific
                *result = TError(TError::EType::NOGEOFOUND, "No geo found for user country");
            }
        } break;

        case EType::HOME:
        case EType::WORK: {
            TSavedAddress sa = ctx.GetSavedAddress(*this);
            if (!sa.IsNull()) {
                *result = LLToGeo(ctx, sa.Latitude(), sa.Longitude(), &geoId);
            } else {
                // XXX this is a temporary solution.
                // it is better to force ctx.GetSavedAddress() returns a real status if it is not valid.
                // Now we always decide that if special location either work or home, error will be nosaveaddress
                // (which is usefull to switch a special form, see <TSaveAddressHandler::SetAsResponse()>)
                *result = TError(TError::EType::NOSAVEDADDRESS);
            }
        } break;

        case EType::ERROR:
            *result = TError(TError::EType::NOTSUPPORTED, AsString());
            break;
    }

    return geoId;
}

TSpecialLocation TSpecialLocation::GetNamedLocation(const TSlot* slot) {
    if (!slot) {
        return TSpecialLocation(TSpecialLocation::EType::ERROR);
    }
    const auto& location = NAlice::TSpecialLocation::GetNamedLocation(slot->Value.GetString(), slot->Type);
    return TSpecialLocation(location.Value);
}

}

template <>
void Out<NBASS::TSpecialLocation>(IOutputStream& out, const NBASS::TSpecialLocation& sl) {
    out << sl.AsString();
}
