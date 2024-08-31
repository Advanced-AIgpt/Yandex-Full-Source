#pragma once

#include <alice/bass/libs/smallgeo/kdtree.h>

#include <library/cpp/scheme/scheme.h>
#include <util/generic/hash.h>

#include <cmath>

namespace NBASS {
class T2DFMRadioPoint : public NSmallGeo::I2DTreePoint, public TThrRefBase {
public:
    // lat, lon = x, y
    T2DFMRadioPoint(double x, double y, i32 regionId)
        : X(x)
        , Y(y)
        , GeoId(regionId)
    {
    }

    double GetX() const override {
        return X;
    }

    double GetY() const override {
        return Y;
    }

    i32 GetGeoId() const {
        return GeoId;
    }

private:
    double X;
    double Y;
    i32 GeoId;
};


template <>
struct NSmallGeo::T2DTreeMetric<T2DFMRadioPoint> {
    double Distance(double x1, double y1, double x2, double y2) {
        // we dont need a square root here
        return ((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
    }
};


namespace NAutomotive {

struct TPrevNextStations {
    TString Prev;
    TString Next;
};

class TFMRadioDatabase {
private:
    THashMap<i32, THashMap<TString, TString>> RadioByName;
    THashMap<i32, THashMap<TString, TString>> RadioByFreq;
    THashMap<i32, THashMap<TString, TPrevNextStations>> RadioPrevNext;
    THolder<NSmallGeo::T2DTree<T2DFMRadioPoint>> RadioRegionTree;

public:
    TFMRadioDatabase();
    i32 GetNearest(double lat, double lon) const;
    bool HasRegion(i32 regionId) const;
    bool HasRadioByRegion(i32 regionId, const TString& radio) const;
    TString GetRadioByRegion(i32 regionId, const TString& radio) const;
    TMaybe<TString> SeekRadioByRegion(i32 regionId, const TString& radio, bool forward = true) const;
    size_t GetNumStations(i32 regionId) const;
};


} // namespace NAutomotive
} // namespace NBASS




