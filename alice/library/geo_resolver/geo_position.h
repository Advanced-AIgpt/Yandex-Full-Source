#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>

namespace NAlice {

struct TGeoPosition {
    double Lat;
    double Lon;

    TGeoPosition()
        : Lat(0)
        , Lon(0)
    {}

    TGeoPosition(double lat, double lon)
        : Lat(lat)
        , Lon(lon)
    {}

    static TMaybe<TGeoPosition> FromJson(const NSc::TValue& location);

    TString GetLonLatString() const {
        return TString::Join(ToString(Lon), ",", ToString(Lat));
    }

    NSc::TValue ToJson() const {
        NSc::TValue location;
        location["lat"] = Lat;
        location["lon"] = Lon;
        return location;
    }
};

}
