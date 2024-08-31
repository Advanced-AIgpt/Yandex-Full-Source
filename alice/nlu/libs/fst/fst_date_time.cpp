#include "fst_date_time.h"

#include <util/charset/unidata.h>
#include <util/string/strip.h>

#include <time.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>

namespace NAlice {

    static const TMap<char, TString> TYPES{
        {'S', "seconds"},
        {'M', "minutes"},
        {'H', "hours"},
        {'d', "days"},
        {'w', "weeks"},
        {'m', "months"},
        {'Y', "years"},
        {'W', "weekday"}
    };

    static re2::RE2::Options MakeOptions() {
        re2::RE2::Options opts{};
        opts.set_longest_match(true);
        return opts;
    }

    TVector<TStringBuf> TFstDateTime::FindValues(const TStringBuf& tape) const {
        TVector<TStringBuf> values;
        using namespace re2;
        StringPiece input{tape.data(), tape.size()};
        StringPiece value;
        const auto& tapePattern = TapePattern();
        while (RE2::FindAndConsume(&input, tapePattern, &value)) {
            values.emplace_back(value.data(), value.size());
        }
        return values;
    }

    TParsedToken::TValueType PrettyOut(const TMap<char, int64_t>& out, const TMultiMap<char, int64_t>& relOut) {
        NSc::TValue value;
        for (const auto& pair : out) {
            const auto& type  = TYPES.at(pair.first);
            value.Add(type) = pair.second;
        }
        const TString relativeSuffix = "_relative";
        for (const auto& pair : relOut) {
            auto type = TYPES.at(pair.first);
            if (auto f = value.GetDictMutable().find(type); f != std::end(value.GetDictMutable())) {
                f->second.GetIntNumberMutable() += pair.second;
            } else {
                value.Add(type) = pair.second;
            }
            TString relativeKey = type + relativeSuffix;
            value.Add(relativeKey).SetBool(true);
        }

        size_t count = 0;
        std::array<const char*, 3> timeStrs = {"hours", "minutes", "seconds"};
        for (auto str : timeStrs) {
            count += value.GetDict().count(str + relativeSuffix);
        }
        if (count == timeStrs.size()) {
            value.Add("time_relative").SetBool(true);
            for (auto str : timeStrs) {
                value.Delete(str + relativeSuffix);
            }
        }

        count = 0;
        std::array<const char*, 3> dateStrs = {"days", "months", "years"};
        for (auto str : dateStrs) {
            count += value.GetDict().count(str + relativeSuffix);
        }
        if (count == dateStrs.size()) {
            value.Add("date_relative").SetBool(true);
            for (auto str : dateStrs) {
                value.Delete(str + relativeSuffix);
            }
        }

        return value;
    }

    int64_t GetDateTimeAttrByFmt(const tm& time, const char key) {
        switch (key) {
            case 'S':
                return time.tm_sec;
            case 'M':
                return time.tm_min;
            case 'H':
                return time.tm_hour;
            case 'd':
                return time.tm_mday;
            case 'm':
                return time.tm_mon + 1; // range [1-12]
            case 'Y':
                return time.tm_year + 1900;
            case 'W':
                return time.tm_wday;
            default:
                Y_ENSURE(false, "Unsupported format key");
        }
    }

    struct TDelta {
        using days = std::chrono::duration<int64_t, std::ratio<86400>>;
        using weeks = std::chrono::duration<int64_t, std::ratio<604800>>;
        using months = std::chrono::duration<int64_t, std::ratio<2629746>>;
        using years = std::chrono::duration<int64_t, std::ratio<31556952>>;

        std::chrono::seconds Seconds{0};
        std::chrono::minutes Minutes{0};
        std::chrono::hours Hours{0};
        days Days{0};
        months Months{0};
        years Years{0};
    };

    void Sum(const TDelta& delta, tm* time) {
        time->tm_sec += delta.Seconds.count();
        time->tm_min += delta.Minutes.count();
        time->tm_hour += delta.Hours.count();
        time->tm_mday += delta.Days.count();
        time->tm_mon += delta.Months.count();
        time->tm_year += delta.Years.count();
        mktime(time);
    }

    const RE2& TFstDateTime::TapePattern() const {
        static const RE2 tapePattern{R"((\d+\s+\w[\+\-]?))", MakeOptions()};
        Y_ENSURE(tapePattern.ok());
        return tapePattern;
    }

    TParsedToken::TValueType TFstDateTime::ApplyRelOut(TMap<char, int64_t> out, TMultiMap<char, int64_t> wholeRelOut) {
        auto relOut = wholeRelOut;
        for (auto it = begin(relOut); it != end(relOut); ) {
            if (out.find(it->first) == end(out)) {
                relOut.erase(it++);
                continue;
            }
            ++it;
        }

        if (out.empty() || relOut.empty()) {
            return PrettyOut(out, wholeRelOut);
        }

        if (auto it = relOut.find('H'); it != end(relOut) && out.at('H') >= 12) {
            while (it != end(relOut) && it->first == 'H') {
                if (it->second == 12) {
                    relOut.erase(it++);
                } else {
                    ++it;
                }
            }
        }

        TDelta delta;
        for (const auto& keyValue : relOut) {
            using namespace std::chrono;
            switch (keyValue.first) {
            case 'S':
                delta.Seconds += seconds(keyValue.second);
                break;
            case 'M':
                delta.Minutes += minutes(keyValue.second);
                break;
            case 'H':
                delta.Hours += hours(keyValue.second);
                break;
            case 'd':
                delta.Days += TDelta::days(keyValue.second);
                break;
            case 'w':
                delta.Days += TDelta::weeks(keyValue.second);
                break;
            case 'm':
                delta.Months += TDelta::months(keyValue.second);
                break;
            case 'Y':
                delta.Years += TDelta::years(keyValue.second);
                break;
            default:
                Y_ENSURE(false);
            }
        }

        TString dateVal;
        TString dateFmt;
        for (const auto& pair : out) {
            auto strVal = ToString(pair.second);
            if (strVal.size() == 1
                && (pair.first == 'H'
                    || pair.first == 'M'
                    || pair.first == 'S'))
            {
                strVal.insert(strVal.begin(), '0');
            }
            if (!dateVal.empty()) {
                dateVal.push_back(' ');
                dateFmt.push_back(' ');
            }
            dateVal += strVal;
            dateFmt.push_back('%');
            dateFmt.push_back(pair.first);
        }

        struct tm t{};
        strptime(dateVal.c_str(), dateFmt.c_str(), &t);

        Sum(delta, &t);

        for (auto& pair : out) {
            if (pair.first == 'W') {
                continue;
            }
            pair.second = GetDateTimeAttrByFmt(t, pair.first);
        }

        relOut.clear();
        for (const auto& pair : wholeRelOut) {
            if (out.find(pair.first) == end(out)) {
                relOut.insert(pair);
            }
        }

        return PrettyOut(out, relOut);
    }

    TParsedToken::TValueType TFstDateTime::ToDateTime(const TVector<TStringBuf>& values) const {
        static const RE2 pattern{R"((\d+)\s+(\w)([\-\+]?))", MakeOptions()};
        Y_ENSURE(pattern.ok());
        int64_t v;
        char sign = '\0';
        char f;
        TMap<char, int64_t> out;
        TMultiMap<char, int64_t> relOut;
        for (const auto& value : values) {
            sign = '\0';
            re2::StringPiece input{value.data(), value.size()};
            if (RE2::FindAndConsume(&input, pattern, &v, &f, &sign)
                    || RE2::FindAndConsume(&input, pattern, &v, &f))
            {
                if (sign != '\0') {
                    relOut.emplace(f, (sign == '+') ? v : -v);
                } else {
                    if (f == 'Y' && v / 100 == 0) {
                        if (v <= 40) {
                            v += 2000;
                        } else {
                            v += 1900;
                        }
                    }
                    out[f] = v;
                }
            } else {
                Y_ENSURE(false);
            }
        }

        return ApplyRelOut(out, relOut);
    }

    bool AllDigits(TStringBuf str) {
        if (str.empty()) {
            return false;
        }
        for (auto ch : str) {
            if (!IsDigit(ch)) {
                return false;
            }
        }
        return true;
    }

    TParsedToken::TValueType TFstDateTime::ParseValue(
        const TString& type, TString* stringValue, TMaybe<double>* weight) const
    {
        if (stringValue->find(V_BEG) == TString::npos) {
            return TFstBase::ParseValue(type, stringValue, weight);
        }

        auto& substr = StripInPlace(*stringValue);
        TString outputTape;
        {
            TString newSubstr;
            bool match = false;
            size_t start = 0u;
            for (auto i = 0u; i < substr.size(); ++i) {
                if (match == false && substr[i] == V_BEG) {
                    match = true;
                    start = i + 1u;
                    continue;
                } else if (match == true && substr[i] == V_END) {
                    match = false;
                    TStringBuf matched{substr.c_str() + start, substr.c_str() + i};
                    outputTape += matched;
                    if (AllDigits(matched)) {
                        newSubstr += matched;
                    }
                    continue;
                }
                if (match == false) {
                    newSubstr.push_back(substr[i]);
                }
            }
            substr = std::move(newSubstr);
        }

        auto values = FindValues(outputTape);

        return ToDateTime(values);
    }

} // namespace NAlice
