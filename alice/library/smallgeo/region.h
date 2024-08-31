#pragma once

#include "kdtree.h"
#include "latlon.h"
#include "utils.h"

#include <library/cpp/geobase/lookup.hpp>

#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>
#include <util/ysaveload.h>

#include <cstddef>
#include <utility>

class IInputStream;
class IOutputStream;

namespace NAlice::NSmallGeo {

// Compact verison of NGeobase::TRegion class. Contains only what do
// we need.
struct TRegion : public I2DTreePoint, public TThrRefBase {
    using TId = NGeobase::TId;

    TRegion() = default;
    TRegion(const TRegion&) = default;
    TRegion(const NGeobase::TRegion& region, const NGeobase::TLinguistics& lings);

    template <typename TFn>
    void ForEachName(TFn&& fn) const {
        fn(Name);

        for (const auto& ling : Linguistics)
            fn(ling);

        for (const auto& synonym : Synonyms)
            fn(synonym);
    }

    template <typename TFn>
    void ForEachToken(TFn&& fn) const {
        ForEachName([&fn](TStringBuf name) { ::NAlice::NSmallGeo::ForEachToken(name, fn); });
    }

    double GetX() const {
        return Center.Lat;
    }

    double GetY() const {
        return Center.Lon;
    }

    TString Name;
    TVector<TString> Linguistics;
    TVector<TString> Synonyms;

    TId Id = 0;
    TId ParentId = 0;
    int Type = 0;

    TLatLon Center;

    Y_SAVELOAD_DEFINE(Name, Linguistics, Synonyms, Id, ParentId, Type, Center);
};

template <>
struct T2DTreeMetric<TRegion> {
    double Distance(double x1, double y1, double x2, double y2) {
        return TLatLon(x1, y1).DistanceTo(TLatLon(x2, y2));
    }
};

struct TFixElement {
    TFixElement(TString regexp, TLatLon position, TString preposition, TString prepcase, NGeobase::TId parentId)
        : Regexp(regexp)
        , Position(position)
        , Preposition(preposition)
        , Prepcase(prepcase)
        , ParentId(parentId)
    {
    }

    TString Regexp;
    TLatLon Position;
    TString Preposition;
    TString Prepcase;
    NGeobase::TId ParentId;
};

class TRegions : public NNonCopyable::TNonCopyable {
public:
    using TStorage = TVector<TRegion>;
    using TId = NGeobase::TId;

public:
    explicit TRegions(IInputStream& in);

    const TStorage& Storage() const {
        return Regions;
    }

    TMaybe<TId> FindCityWithSameName(TId id);

    TMaybe<TString> FindRegionInFixList(TStringBuf name);

    TMaybe<TFixElement> FindLatLonInFixList(TStringBuf name);

    void SaveToStream(IOutputStream& output, TVector<TRegion>& regions);

    static TStorage LoadFromStream(IInputStream& input);

private:
    // Maps id of a bigger region to a city with the same name.
    TMap<TId, TId> RegionsWithSameNames;

    // All regions.
    TStorage Regions;
};

} // namespace NAlice::NSmallGeo
