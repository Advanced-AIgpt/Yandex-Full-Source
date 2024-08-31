#pragma once

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NBASS {

namespace NMarket {

struct TRawGlFilter {
    TRawGlFilter() {}
    TRawGlFilter(TStringBuf id, const TVector<TString>& values)
        : Id(ToString(id))
        , Values(values)
    {
    }

    TString Id;
    TVector<TString> Values;
};

struct TBoolGlFilter {
    TBoolGlFilter() {}
    TBoolGlFilter(TStringBuf id, bool value)
        : Id(ToString(id))
        , Value(value)
    {
    }
    TBoolGlFilter(TStringBuf id, TStringBuf name, bool value)
        : Id(ToString(id))
        , Name(ToString(name))
        , Value(value)
    {
    }

    TString Id;
    TString Name = "";
    bool Value;
};

struct TNumberGlFilter {
    TNumberGlFilter() {}
    TNumberGlFilter(TStringBuf id, TMaybe<double> min, TMaybe<double> max)
        : Id(ToString(id))
        , Min(min)
        , Max(max)
    {
    }
    TNumberGlFilter(TStringBuf id, TStringBuf name, TMaybe<double> min, TMaybe<double> max, TStringBuf unit)
        : Id(ToString(id))
        , Name(ToString(name))
        , Min(min)
        , Max(max)
        , Unit(ToString(unit))
    {
    }

    TString Id;
    TString Name = "";
    TMaybe<double> Min;
    TMaybe<double> Max;
    TString Unit = "";
};

struct TEnumGlFilter {
    struct TValue {
        TValue() {}
        TValue(TStringBuf id)
            : Id(id)
        {
        }
        TValue(TStringBuf id, TStringBuf name)
            : Id(id)
            , Name(ToString(name))
        {
        }
        TString Id;
        TString Name = "";
    };
    class TValues : public THashMap<TString, TValue> {
    public:
        void Add(const TValue& val)
        {
            operator[](val.Id) = val;
        }
    };
    TEnumGlFilter() {}
    TEnumGlFilter(TStringBuf id, const TValues& values)
        : Id(ToString(id))
        , Values(values)
    {
    }
    TEnumGlFilter(TStringBuf id, TStringBuf name, const TValues& values)
        : Id(ToString(id))
        , Name(ToString(name))
        , Values(values)
    {
    }

    TString Id;
    TString Name = "";
    TValues Values;
};

using TGlFilter = std::variant<TBoolGlFilter, TNumberGlFilter, TEnumGlFilter, TRawGlFilter>;
class TGlFilters : public THashMap<TString, TGlFilter> {
public:
    template <class TFilter>
    void Add(const TFilter& filter)
    {
        operator[](filter.Id) = filter;
    }
};

TString ToDebugString(const TGlFilter& filter);

} // namespace NMarket

} // namespace NBASS
