#include "geo_position.h"

namespace NAlice {

TMaybe<TGeoPosition> TGeoPosition::FromJson(const NSc::TValue& location) {
    TMaybe<TGeoPosition> pos;

    const NSc::TValue lat = location.TrySelect("lat");
    const NSc::TValue lon = location.TrySelect("lon");

    if (!lat.IsNull() && !lon.IsNull()) {
        pos = TGeoPosition();
        pos->Lon = lon.GetNumber();
        pos->Lat = lat.GetNumber();
    }

    return pos;
}

}
