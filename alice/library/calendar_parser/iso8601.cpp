#include "iso8601.h"

#include <library/cpp/timezone_conversion/convert.h>

#include <util/string/ascii.h>

namespace {

class TStreamWrapper {
public:
    explicit TStreamWrapper(IOutputStream& os)
        : Stream(os) {
    }

    template <typename T>
    TStreamWrapper& operator<<(const T& value) {
        Stream << value;
        return *this;
    }

    template <typename T>
    void Emit(TStringBuf name, const T& value) {
        if (!First)
            Stream << ", ";
        Stream << name << ": " << value;
        First = false;
    }

private:
    IOutputStream& Stream;
    bool First = true;
};

template <int Digits>
bool ParseInt(TStringBuf& buffer, int& value) {
    static_assert(Digits >= 0 && Digits < 10, "");
    if (buffer.size() < Digits) {
        return false;
    }
    int v = 0;
    for (size_t i = 0; i < Digits; ++i) {
        const auto c = buffer[i];
        if (!IsAsciiDigit(c)) {
            return false;
        }
        v = v * 10 + (c - '0');
    }
    buffer.Skip(Digits);
    value = v;
    return true;
}

} // namespace

namespace NCalendarParser {

// TRFC8601SerDes --------------------------------------------------------------
// static
TString TISO8601SerDes::Ser(const TInstant& epoch) {
    const NDatetime::TCivilSecond second = NDatetime::Convert(epoch, NDatetime::GetUtcTimeZone());
    return Ser(second) + "Z";
}

// static
TString TISO8601SerDes::Ser(const NDatetime::TCivilSecond& second, const NDatetime::TTimeZone& tz) {
    return Ser(NDatetime::Convert(second, tz));
}

// static
TString TISO8601SerDes::Ser(const NDatetime::TCivilSecond& second) {
    static constexpr size_t DATE_SIZE = 4 + 2 + 2; // YYYYMMDD
    static constexpr size_t TIME_SIZE = 2 + 2 + 2; // HHMMSS

    // +1 for intermediate 'T', +1 for '\0'.
    static constexpr size_t BUF_SIZE = DATE_SIZE + 1 + TIME_SIZE + 1;

    char buf[BUF_SIZE];
    snprintf(buf, BUF_SIZE, "%04d%02d%02dT%02d%02d%02d", static_cast<int>(second.year()), second.month(), second.day(),
             second.hour(), second.minute(), second.second());
    return TString(buf);
}

// static
NDatetime::TCivilDay TISO8601SerDes::DesDate(TStringBuf& stream) {
    int years, months, days;

    TStringBuf s = stream;
    if (!ParseInt<4>(s, years))
        ythrow TException() << "Can't parse years";

    if (!ParseInt<2>(s, months))
        ythrow TException() << "Can't parse months";
    if (months < 1 || months > 12)
        ythrow TException() << "Invalid number of months: " << months;

    if (!ParseInt<2>(s, days))
        ythrow TException() << "Can't parse days";
    if (days < 1 || days > cctz::detail::impl::days_per_month(years, months))
        ythrow TException() << "Invalid number of days: " << days;

    stream = s;
    return NDatetime::TCivilDay{years, months, days};
}

// static
TISO8601SerDes::TDateTime TISO8601SerDes::DesDateTime(TStringBuf& stream) {
    TStringBuf s = stream;

    const auto date = DesDate(s);
    if (!s.StartsWith("T"))
        ythrow TException() << "No T-separator between date and time";
    s.Skip(1);

    int hours, minutes, seconds;
    if (!ParseInt<2>(s, hours))
        ythrow TException() << "Can't parse hours";
    if (hours >= 24)
        ythrow TException() << "Invalid number of hours: " << hours;

    if (!ParseInt<2>(s, minutes))
        ythrow TException() << "Can't parse minutes";
    if (minutes >= 60)
        ythrow TException() << "Invalid number of minutes: " << minutes;

    if (!ParseInt<2>(s, seconds))
        ythrow TException() << "Can't parse seconds";
    if (seconds > 60)
        ythrow TException() << "Invalid number of seconds: " << seconds;
    if (seconds == 60)
        --seconds;

    ETimeZone timeZone = ETimeZone::Unspecified;
    if (s.StartsWith("Z")) {
        timeZone = ETimeZone::UTC;
        s.Skip(1);
    }

    stream = s;
    return {NDatetime::TCivilSecond{date.year(), date.month(), date.day(), hours, minutes, seconds}, timeZone};
}

} // namespace NCalendarParser

template <>
void Out<NCalendarParser::TISO8601SerDes::TDateTime>(IOutputStream& o,
                                                       const NCalendarParser::TISO8601SerDes::TDateTime& dateTime) {
    TStreamWrapper s{o};

    s << "DateTime [";
    s.Emit("Time", dateTime.Time);
    s.Emit("TimeZone", dateTime.TimeZone);
    s << "]";
}
