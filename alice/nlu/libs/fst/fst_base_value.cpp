#include "fst_base_value.h"

#include <util/string/cast.h>
#include <util/string/strip.h>

#include <algorithm>

namespace NAlice {

    TParsedToken::TValueType TFstBaseValue::TryParseNumber(TStringBuf str) const {
        int64_t intResult = 0;
        double doubleResult = 0.0;
        auto strCopy = ToString(str);
        auto comma = std::find(strCopy.begin(), strCopy.vend(), ',');
        if (comma != strCopy.end()) {
            *comma = '.';
        }
        if (TryFromString<int64_t>(strCopy, intResult)) {
            return intResult;
        } else if (TryFromString<double>(strCopy, doubleResult)) {
            return doubleResult;
        }

        return str;
    }

    TParsedToken::TValueType TFstBaseValue::ParseValue(
        const TString& type,
        TString* stringValue,
        TMaybe<double>* weight) const
    {
        if (stringValue->find(V_BEG) == TString::npos) {
            return TFstBase::ParseValue(type, stringValue, weight);
        }

        auto substr = *stringValue;
        StripInPlace(substr);
        NSc::TValue values;

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
                    values.Push(TryParseNumber(StripStringLeft(matched, EqualsStripAdapter(V_INSERTED))));
                }
            }
        }

        TString out;
        size_t match = 0u;
        for (auto i = 0u; i < substr.size(); ++i) {
            switch (substr[i]) {
            case V_BEG:
                if (match == 0) {
                    match = 1u;
                }
                continue;
            case V_INSERTED:
                if (match == 1u && substr[i-1] == V_BEG) {
                    match = 2u;
                }
                continue;
            case V_END:
                if (match == 2u) {
                    match = 0u;
                }
                continue;
            }
            if (match == 1u) {
                match = 0u;
            }
            if (match != 2u) {
                out.push_back(substr[i]);
            }
        }

        *stringValue = std::move(out);
        if (values.ArraySize() == 1u) {
            return values.Front();
        }
        return values;
    }

} // namespace NAlice
