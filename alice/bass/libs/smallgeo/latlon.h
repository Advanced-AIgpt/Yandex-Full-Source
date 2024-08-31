#pragma once

#include <library/cpp/geolocation/calcer.h>

#include <util/ysaveload.h>

namespace NBASS {
namespace NSmallGeo {

struct TLatLon {
    static constexpr double MIN_LAT = -90;
    static constexpr double MAX_LAT = +90;

    static constexpr double MIN_LON = -180;
    static constexpr double MAX_LON = +180;

    TLatLon() = default;

    TLatLon(double lat, double lon)
        : Lat(lat)
        , Lon(lon) {
    }

    double DistanceTo(TLatLon position) {
        return NGeolocationFeatures::CalcDistance(Lat, Lon, position.Lat, position.Lon);
    }

    double Lat = 0;
    double Lon = 0;

    Y_SAVELOAD_DEFINE(Lat, Lon);
};

} // namespace NSmallGeo
} // namespace NBASS
