#pragma once

#include <alice/hollywood/library/resources/resources.h>

#include <alice/library/json/json.h>
#include <alice/library/smallgeo/kdtree.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>

namespace NAlice {

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.h?rev=r8680832#L11
class T2DFMRadioPoint : public NSmallGeo::I2DTreePoint, public TThrRefBase {
public:
    // lat, lon = x, y
    T2DFMRadioPoint(double x, double y, i32 regionId)
        : X(x)
        , Y(y)
        , GeoId(regionId)
    {}

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

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.h?rev=r8680832#L40
template <>
struct ::NAlice::NSmallGeo::T2DTreeMetric<T2DFMRadioPoint> {
    double Distance(double x1, double y1, double x2, double y2) {
        // we dont need a square root here
        return ((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
    }
};

namespace NHollywood::NMusic {

const TStringBuf RADIO_STATION_RESOURCE_NAME = "radio_station.json";
const TStringBuf RADIO_STATIONS_RESOURCE_NAME = "radio_stations.json";

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.h?rev=r8680832#L51
struct TPrevNextStations {
    TString Prev;
    TString Next;
};

// https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/radio/fmdb.h?rev=r8680832#L56
class TFmRadioResources {
public:
    TFmRadioResources(
        const NJson::TJsonValue& radioStationJson = {},
        const NJson::TJsonValue& radioStationsJson = {});

    i32 GetNearest(double lat, double lon) const;

    bool HasRegion(i32 regionId) const;

    bool HasFmRadioByRegion(i32 regionId, const TString& radio) const;

    TString GetFmRadioByRegion(i32 regionId, const TString& radio) const;

    TMaybe<TString> SeekFmRadioByRegion(i32 regionId, const TString& radio, bool forward = true) const;

    size_t GetNumStations(i32 regionId) const;

    const THashMap<TString, TString>& GetNameToFmRadioId() const {
        return NameToFmRadioId;
    }

private:
    // radio_station
    THashMap<i32, THashMap<TString, TString>> FmRadioByName;
    THashMap<i32, THashMap<TString, TString>> FmRadioByFreq;
    THashMap<i32, THashMap<TString, TPrevNextStations>> FmRadioPrevNext;
    THolder<NSmallGeo::T2DTree<T2DFMRadioPoint>> FmRadioRegionTree;

    // radio_stations
    THashMap<TString, TString> NameToFmRadioId;
};

} // namespace NHollywood::NMusic

} // namespace NAlice
