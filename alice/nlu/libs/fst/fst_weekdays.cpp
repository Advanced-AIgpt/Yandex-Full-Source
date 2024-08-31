#include "fst_weekdays.h"

#include <util/string/strip.h>

#include <array>

namespace NAlice {


    TParsedToken::TValueType TFstWeekdays::ParseValue(
        const TString& type,
        TString* stringValue,
        TMaybe<double>* weight) const
    {
        if (type != "WEEKDAYS") {
            return TFstBaseValue::ParseValue(type, stringValue, weight);
        }

        auto& substr = *stringValue;
        StripInPlace(substr);
        NSc::TValue values;
        values.SetArray();
        {
            bool match = false;
            size_t start = 0u;
            for (auto i = 0u; i < substr.size(); ++i) {
                if (match == false && substr[i] == V_BEG) {
                    match = true;
                    start = i + 1u;
                } else if (match == true && substr[i] == V_END) {
                    match = false;
                    TStringBuf matched{substr.c_str() + start, substr.c_str() + i};
                    values.Push(StripStringLeft(matched, EqualsStripAdapter(V_INSERTED)));
                }
            }
        }

        auto repeat = false;
        auto isRange = false;
        auto prevDay = 0u;
        std::array<bool, 7> weekdays{};
        for (const auto& baseValue : values.GetArray()) {
            const auto& value = baseValue.GetString();
            for (auto ch : value) {
                if (ch >= '1' && ch <= '7') {
                    const unsigned index = ch - '1';
                    if (isRange) {
                        if (prevDay < index) {
                            for (auto i = prevDay; i <= index; ++i) {
                                weekdays.at(i) = true;
                            }
                        } else {
                            for (auto i = 0u; i < weekdays.size(); ++i) {
                                if (i >= prevDay || i <= index) {
                                    weekdays.at(i) = true;
                                }
                            }
                        }
                        isRange = false;
                    } else {
                        weekdays.at(index) = true;
                    }
                    prevDay = index;
                } else if (ch == '-') {
                    isRange = true;
                } else if (ch == '+') {
                    repeat = true;
                }
            }
        }
        if (isRange) {
            for (auto i = prevDay; i < weekdays.size(); ++i) {
                weekdays.at(i) = true;
            }
        }

        NSc::TValue weekdayList;
        weekdayList.SetArray();
        for (auto i = 0u; i < weekdays.size(); ++i) {
            if (weekdays.at(i)) {
                weekdayList.Push(i + 1);
            }
        }
        NSc::TValue out;
        out.Add("weekdays") = std::move(weekdayList);
        out.Add("repeat").SetBool(repeat);

        return out;
    }

} // namespace NAlice
