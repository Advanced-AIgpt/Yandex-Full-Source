#include "fst_date_time_range.h"

#include <re2/re2.h>

namespace NAlice {

    static re2::RE2::Options MakeOptions() {
        re2::RE2::Options opts{};
        opts.set_longest_match(true);
        return opts;
    }

    const RE2& TFstDateTimeRange::TapePattern() const {
        static const RE2 tapePattern{R"(((?:s|e):(?:\d+\s+\w[\+\-]?)+))", MakeOptions()};
        Y_ENSURE(tapePattern.ok());
        return tapePattern;
    }

    TParsedToken::TValueType TFstDateTimeRange::ToDateTime(const TVector<TStringBuf>& values) const {
        static const RE2 pattern{R"((\d+)\s+(\w)([\-\+]?))", MakeOptions()};
        TParsedToken::TValueType output;
        int64_t v;
        char f;
        for (const auto& value : values) {
            TMap<char, int64_t> out;
            TMultiMap<char, int64_t> relOut;
            bool weekend = false;
            bool holidays = false;
            char sign = '\0';
            re2::StringPiece input{value.data(), value.size()};
            while (RE2::FindAndConsume(&input, pattern, &v, &f, &sign)
                ||  RE2::FindAndConsume(&input, pattern, &v, &f))
            {
                if (f == 'E') {
                    weekend = true;
                    f = 'w';
                }
                if (f == 'O') {
                    holidays = true;
                } else {
                    if (sign != '\0') {
                        relOut.emplace(f, (sign == '+') ? v : -v);
                    } else {
                        out[f] = v;
                    }
                }
            }
            auto outValue = ApplyRelOut(out, relOut);
            if (weekend) {
                outValue.Add("weekend").SetBool(true);
            }
            if (holidays) {
                outValue.Add("holidays").SetBool(true);
            }

            if (value.StartsWith('s')) {
                output.Add("start") = std::move(outValue);
            } else if (value.StartsWith('e')) {
                output.Add("end") = std::move(outValue);
            }
        }

        if (!output.Has("start")) {
            auto& start = output.Add("start") = output.Get("end");
            for (auto& pair : start.GetDictMutable()) {
                if (pair.second.IsIntNumber()
                    && !pair.second.IsBool()
                    && !pair.first.EndsWith("_relative"))
                {
                    pair.second = 0;
                }
            }
        }

        return output;
    }

} // namespace NAlice

