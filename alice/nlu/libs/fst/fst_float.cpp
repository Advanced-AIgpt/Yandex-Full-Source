#include "fst_float.h"

#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>

#include <cmath>

namespace NAlice {

    TParsedToken::TValueType TFstFloat::TryParseNumber(TStringBuf str) const {
        auto&& value = TFstBaseValue::TryParseNumber(str);
        if (!value.IsString()) {
            return value;
        }

        auto stringValue = Collapse(Strip(ToString(value.GetString())));
        if (stringValue.find('/') != TStringBuf::npos) {
            double nom;
            double denom;
            try {
                TString noSpaces;
                for (auto ch : stringValue) {
                    if (ch == ' ') {
                        continue;
                    }
                    noSpaces.push_back(ch);
                }
                Split(noSpaces, '/', nom, denom);
                if (denom == 0.0) {
                    return value;
                }
                return nom / denom;
            } catch (const yexception&) {
                return value;
            }
        }

        auto&& items = StringSplitter(stringValue).SplitBySet(" \t\n\r").ToList<TString>();
        TString* pIntPart = nullptr;
        TString* pFracPart = nullptr;
        if (items.size() == 2) {
            pIntPart = &items[0];
            pFracPart = &items[1];
        } else if (items.size() == 3 && items[1] == "и") {
            pIntPart = &items[0];
            pFracPart = &items[2];
        }
        double intPart;
        double fracPart;
        if (pIntPart
            && pFracPart
            && TryFromString(*pIntPart, intPart)
            && TryFromString(*pFracPart, fracPart))
        {
            return intPart + fracPart / std::pow(10, pFracPart->size());
        }

        return value;
    }

} // namespace Nalice
