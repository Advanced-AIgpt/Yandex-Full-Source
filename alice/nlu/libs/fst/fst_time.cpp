#include "fst_time.h"

#include "fst_date_time.h"

#include <re2/re2.h>

namespace NAlice {

    static re2::RE2::Options MakeOptions() {
        re2::RE2::Options opts{};
        opts.set_longest_match(true);
        return opts;
    }

    static RE2 UNIT_PATTERN{R"((?P<format>[SMH])(?P<sign>[\-\+]?)|(?P<period>am|pm|morning|day|evening|night))", MakeOptions()};

    static TString NormalizePeriod(TStringBuf period, int hours) {
        // There is nothing special in constant 7. It's just my perception of day and night.
        if (period == "morning") {
            return hours == 12 ? "pm" : "am";
        }
        if (period == "day") {
            return (hours == 12 || hours <= 7) ? "pm" : "am";
        }
        if (period == "evening") {
            return hours == 12 ? "am" : "pm";
        }
        if (period == "night") {
            return (hours == 12 || hours <= 7) ? "am" : "pm";
        }
        return TString(period);
    }

    TParsedToken::TValueType TFstTime::ProcessValue(const TParsedToken::TValueType& value) const
    {
        if (!value.IsArray()) {
            return value;
        }

        TMap<char, int64_t> out;
        TMultiMap<char, int64_t> relOut;
        TMaybe<int64_t> num;
        TMaybe<TString> permanentPeriod;
        for (const auto& item : value.GetArray()) {
            TString format;
            TString sign;
            TString period;
            if (item.IsIntNumber()) {
                num = item.GetIntNumber();
            } else {
                const auto& str = item.GetString();
                re2::StringPiece piece{str.data(), str.size()};
                if (RE2::FullMatch(piece, UNIT_PATTERN, &format, &sign, &period)) {
                    if (!format.Empty()) {
                        Y_ENSURE(num.Defined(), "Tape invalid format");
                        if (!sign.Empty()) {
                            relOut.emplace(format.front(), sign.front() == '+' ? *num : -*num);
                        } else {
                            out[format.front()] = *num;
                        }
                    }
                    if (!period.Empty()) {
                        permanentPeriod = std::move(period);
                    }
                }
                num.Clear();
            }
        }

        auto result = TFstDateTime::ApplyRelOut(out, relOut);
        if (permanentPeriod.Defined()) {
            if (auto value = result.GetNoAdd("hours"); value && value->GetIntNumber() == 0) {
                *value = 12;
            }
            result.Add("period") = NormalizePeriod(*permanentPeriod, result.Get("hours").GetIntNumber(0));
        }

        return result;
    }

} // namespace NAlice
