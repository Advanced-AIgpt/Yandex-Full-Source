#include "user_location.h"

#include <alice/library/geo/protos/user_location.pb.h>

#include <util/generic/maybe.h>

namespace NAlice {
namespace {

TString NormalizeTimeZone(TString userTimeZone) {
    static const TString TIMEZONE_FALLBACK = "Europe/Moscow";

    // TODO (petrk, mihajlova) Add checking if tz is valid TimeZone.

    if (userTimeZone.Empty()) {
        userTimeZone = TIMEZONE_FALLBACK;
    }

    return userTimeZone;
}

NGeobase::TId GetCountryId(const NGeobase::TLookup& geobase, const NGeobase::TId regId) {
    if (IsValidId(regId)) {
        const NGeobase::TId countryId = geobase.GetCountryId(regId);
        return IsValidId(countryId) ? countryId : regId;
    }
    return NGeobase::UNKNOWN_REGION;
}

} // namespace

TUserLocation::TUserLocation(const TString& userTimeZone, const TString& tld)
    : UserRegion_(NGeobase::UNKNOWN_REGION)
    , UserCountry_(NGeobase::UNKNOWN_REGION)
    , UserTld_(tld)
    , UserTimeZone_(NormalizeTimeZone(userTimeZone))
{
}

TUserLocation::TUserLocation(const NGeobase::TLookup& geobase, NGeobase::TId userRegion, const TString& userTimeZone)
    : UserRegion_(userRegion)
    , UserCountry_(GetCountryId(geobase, UserRegion_))
    , UserTld_(GeoIdToTld(geobase, UserRegion_))
    , UserTimeZone_(userTimeZone)
{
    if (!UserTimeZone_) {
        if (IsValidId(UserRegion_)) {
            UserTimeZone_ = geobase.GetTimezoneName(UserRegion_);
        } else {
            UserTimeZone_ = NormalizeTimeZone(std::move(UserTimeZone_));
        }
    }
}

TUserLocation::TUserLocation(const TUserLocationProto& locationProto, const TString& userTimeZone)
    : UserRegion_(locationProto.GetUserRegion())
    , UserCountry_(locationProto.GetUserCountry())
    , UserTld_(locationProto.GetUserTld())
    , UserTimeZone_(userTimeZone)
{
}

TUserLocation::TUserLocation(const TString& userTld, NGeobase::TId userRegion, const TString& userTimeZone, NGeobase::TId userCountry)
    : UserRegion_(userRegion)
    , UserCountry_(userCountry)
    , UserTld_(userTld)
    , UserTimeZone_(NormalizeTimeZone(userTimeZone))
{
}

TUserLocationProto TUserLocation::BuildProto() const {
    TUserLocationProto proto;
    proto.SetUserRegion(UserRegion_);
    proto.SetUserTld(UserTld_);
    if (IsValidId(UserCountry_)) {
        proto.SetUserCountry(UserCountry_);
    }
    return proto;
}

} // namespace NAlice
