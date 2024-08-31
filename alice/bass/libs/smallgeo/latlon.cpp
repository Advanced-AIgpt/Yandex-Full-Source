#include "latlon.h"

#include <util/stream/output.h>

template <>
void Out<NBASS::NSmallGeo::TLatLon>(IOutputStream& o, const NBASS::NSmallGeo::TLatLon& p) {
    o << p.Lat << ", " << p.Lon;
}
