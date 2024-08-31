#pragma once

#include <library/cpp/timezone_conversion/civil.h>

namespace NCalendarParser {

class TISO8601SerDes final {
public:
    enum class ETimeZone { UTC, Unspecified };

    struct TDateTime {
        TDateTime() = default;

        TDateTime(const NDatetime::TCivilSecond& time, ETimeZone timeZone)
            : Time(time)
            , TimeZone(timeZone) {
        }

        bool operator==(const TDateTime & rhs) const {
            return Time == rhs.Time && TimeZone == rhs.TimeZone;
        }

        NDatetime::TCivilSecond Time;
        ETimeZone TimeZone = ETimeZone::Unspecified;
    };

    struct TException : public yexception {};

    // Converts |time| to UTC and serializes to compact representation
    // in accordance with ISO8601.
    static TString Ser(const TInstant& epoch);
    static TString Ser(const NDatetime::TCivilSecond& second, const NDatetime::TTimeZone& tz);

    // Serializes to compact representation in accordance with ISO8601.
    static TString Ser(const NDatetime::TCivilSecond& second);

    // Parses serialized date. In case of issues, throws TException.
    // Otherwise, skips date part of the |stream| and returns parsed
    // date.
    static NDatetime::TCivilDay DesDate(TStringBuf& stream);

    // Parses serialized date/time. In case of issues, throws
    // TException.  Otherwise, skips date-time part of the |stream|
    // and returns parsed date.
    //
    // NOTE: this implementation does not handle leap seconds. They're
    // always transformed to 59 seconds.
    static TDateTime DesDateTime(TStringBuf& stream);
};

} // namespace NCalendarParser
