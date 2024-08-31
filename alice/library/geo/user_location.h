#pragma once

#include <alice/library/geo/geodb.h>

#include <util/generic/string.h>

namespace NAlice {

class TUserLocationProto;

class TUserLocation {
public:
    TUserLocation(const TString& userTimeZone, const TString& tld);
    TUserLocation(const TUserLocationProto& locationProto, const TString& userTimeZone);
    TUserLocation(const NGeobase::TLookup& geobase, NGeobase::TId userRegion, const TString& userTimeZone);

    // For unit tests
    TUserLocation(const TString& userTld, NGeobase::TId userRegion, const TString& userTimeZone, NGeobase::TId userCountry);

    NGeobase::TId UserRegion() const {
        return UserRegion_;
    }

    NGeobase::TId UserCountry() const {
        return UserCountry_;
    }

    const TString& UserTld() const {
        return UserTld_;
    }

    const TString& UserTimeZone() const {
        return UserTimeZone_;
    }

    TUserLocationProto BuildProto() const;

private:
    NGeobase::TId UserRegion_;
    NGeobase::TId UserCountry_;
    TString UserTld_;
    TString UserTimeZone_;
};

} // namespace NAlice
