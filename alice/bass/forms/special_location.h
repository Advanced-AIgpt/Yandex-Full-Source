#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/util/error.h>

#include <alice/library/geo/geodb.h>
#include <alice/library/special_location/special_location.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NBASS {
/// Helper "enum class" for special location with methods.
struct TSpecialLocation : public NAlice::TSpecialLocation {

    using EType = NAlice::TSpecialLocation::EType;

    explicit TSpecialLocation(TStringBuf str)
        : NAlice::TSpecialLocation(str) {
    }

    explicit TSpecialLocation(EType value)
        : NAlice::TSpecialLocation(value) {
    }

    /** It tries to convert a special location to a geoid base on specail location type!
     * If special location is not supported it returns NGeobase::UNKNOWN_REGION and set <result> as NONSUPPORTED.
     * NB: for WORK and HOME in case if TSavedLoaction != IsNull() return -1 and set error to NOSAVEDADDRESS.
     * It allows you to switch to a different form which asks user about its location (home/work)
     * @param[in] ctx is a context which is needed to obtain some user info
     * @param[out] result is where error is stored if something bad happened
     * @return real geo id if success otherwise NGeobase::UNKNOWN_REGION
     */
    NGeobase::TId GetGeo(TContext& ctx, TResultValue* result) const;

    inline bool IsNearLocation() const {
        return NAlice::TSpecialLocation::IsNearLocation();
    }

    /** Get enum value of named special location if any.
     */
    static TSpecialLocation GetNamedLocation(const TSlot* slot);

    /** Check, whether slot contains any special location
     */
    static bool IsSpecialLocation(const TSlot* slot) {
        return GetNamedLocation(slot) != TSpecialLocation::EType::ERROR;
    }

    /** Check, whether slot contains special value for "near" location
     */
    static bool IsNearLocation(const TSlot* slot) {
        return GetNamedLocation(slot).IsNearLocation();
    }
};

}
