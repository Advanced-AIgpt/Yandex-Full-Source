#pragma once

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>

#include <alice/hollywood/library/framework/framework.h>

#include <alice/library/sys_datetime/sys_datetime.h>

namespace NAlice::NHollywoodFw::NGetDate {

constexpr TStringBuf SLOT_TARGET_QUESTION = "query_target";
constexpr TStringBuf SLOT_TENSE_VERB = "tense";
constexpr TStringBuf SLOT_CALENDAR_DATE = "calendar_date";
constexpr TStringBuf SLOT_NUMERIC_DATE = "numeric_date";

// Geoposition
constexpr TStringBuf SLOT_WHERE_LOCATION = "where";
constexpr TStringBuf SLOT_WHERE_SLOT_TYPE_ADDRESS = "GeoAddr.Address";
constexpr TStringBuf SLOT_WHERE_SLOT_TYPE_SYS_GEO = "sys.geo";

struct TGetDateFrame : public TFrame {
    TGetDateFrame(const TRequest::TInput& input, const TStringBuf frameName)
        : TFrame(input, frameName)
        , CalerdarDates(this, SLOT_CALENDAR_DATE)
        , NumericDates(this, SLOT_NUMERIC_DATE)
        , Tense(this, SLOT_TENSE_VERB)
        , QueryTarget(this, SLOT_TARGET_QUESTION)
        , Location(this, SLOT_WHERE_LOCATION)
    {
    }
    TArraySlot<TSysDatetimeParser> CalerdarDates;
    TOptionalSlot<i32> NumericDates;
    TOptionalSlot<TString> Tense;
    TArraySlot<TString> QueryTarget;
    // TODO [DD] need to replace with GeoParser in future
    TOptionalSlot<TString> Location;
};

}  // namespace NAlice::NHollywood::NGetDate
