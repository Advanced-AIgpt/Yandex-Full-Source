#include "latlon.h"

#include <util/stream/output.h>

template <>
void Out<NAlice::NSmallGeo::TLatLon>(IOutputStream& o, const NAlice::NSmallGeo::TLatLon& p) {
    o << p.Lat << ", " << p.Lon;
}
