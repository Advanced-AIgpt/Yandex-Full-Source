#pragma once

#include <alice/library/calendar_parser/iso8601.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NVisitors {

struct TSchemeVisitorParams {
    // When true, empty arrays won't be added as dictionary values.
    bool SkipEmptyArrays = false;
};

struct TSchemeDictVisitor {
    TSchemeDictVisitor(NSc::TDict& dict, const TSchemeVisitorParams& params)
        : Dict(dict)
        , Params(params) {
    }

    void operator()(const TStringBuf& value, TStringBuf name) {
        Dict[name] = value;
    }

    void operator()(const TString& value, TStringBuf name) {
        Dict[name] = value;
    }

    void operator()(const bool& value, TStringBuf name) {
        Dict[name] = value;
    }

    template <typename TValue>
    void operator()(const TValue& value, TStringBuf name);

    template <typename TValue>
    void operator()(const TVector<TValue>& values, TStringBuf name);

    template <typename TValue>
    void operator()(const TMaybe<TValue>& value, TStringBuf name) {
        if (value)
            (*this)(*value, name);
    }

    void operator()(const NDatetime::TCivilSecond& second, TStringBuf name) {
        Dict[name] = ToString(second);
    }

    void operator()(const NDatetime::TTimeZone& timeZone, TStringBuf name) {
        Dict[name] = timeZone.name();
    }

    void operator()(const TInstant& instant, TStringBuf name) {
        Dict[name] = NCalendarParser::TISO8601SerDes::Ser(instant);
    }

    NSc::TDict& Dict;
    TSchemeVisitorParams Params;
};

struct TSchemeValueVisitor {
    TSchemeValueVisitor(NSc::TValue& value, const TSchemeVisitorParams& params)
        : Value(value)
        , Params(params) {
    }

    void operator()(const TStringBuf& value, TStringBuf /* name */) {
        Value.SetString(value);
    }

    void operator()(const TString& value, TStringBuf /* name */) {
        Value.SetString(value);
    }

    void operator()(const bool& value, TStringBuf /* name */) {
        Value.SetBool(value);
    }

    template <typename TValue>
    void operator()(const TValue& value, TStringBuf /* name */);

    template <typename TValue>
    void operator()(const TVector<TValue>& values, TStringBuf /* name */);

    template <typename TValue>
    void operator()(const TMaybe<TValue>& value, TStringBuf name) {
        if (value)
            (*this)(*value, name);
    }

    void operator()(const TInstant& instant, TStringBuf /* name */) {
        Value.SetString(NCalendarParser::TISO8601SerDes::Ser(instant));
    }

    void operator()(const NDatetime::TWeekday& weekday, TStringBuf /* name */) {
        Value.SetString(ToString(weekday));
    }

    NSc::TValue& Value;
    TSchemeVisitorParams Params;
};

template <typename TValue>
void TSchemeDictVisitor::operator()(const TValue& value, TStringBuf name) {
    NSc::TValue child;
    TSchemeDictVisitor visitor(child.GetDictMutable(), Params);
    value.Visit(visitor);

    Dict.emplace(name, std::move(child));
}

template <typename TValue>
void TSchemeDictVisitor::operator()(const TVector<TValue>& values, TStringBuf name) {
    if (Params.SkipEmptyArrays && values.empty())
        return;

    NSc::TValue child;
    TSchemeValueVisitor visitor(child, Params);
    visitor(values, name);

    Dict.emplace(name, std::move(child));
}

template <typename TValue>
void TSchemeValueVisitor::operator()(const TValue& value, TStringBuf /* name */) {
    auto& dict = Value.GetDictMutable();
    TSchemeDictVisitor visitor(dict, Params);
    value.Visit(visitor);
}

template <typename TValue>
void TSchemeValueVisitor::operator()(const TVector<TValue>& values, TStringBuf /* name */) {
    auto& array = Value.GetArrayMutable();
    for (const auto& value : values) {
        NSc::TValue item;
        TSchemeValueVisitor visitor(item, Params);
        visitor(value, TStringBuf{} /* name */);
        array.push_back(std::move(item));
    }
}

} // namespace NVisitors
