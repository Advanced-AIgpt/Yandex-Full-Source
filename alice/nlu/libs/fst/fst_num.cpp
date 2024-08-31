#include "fst_num.h"

namespace NAlice {

    TParsedToken::TValueType TFstNum::ParseValue(
        const TString& type
        , TString* stringValue
        , TMaybe<double>* weight) const
    {
        auto value = TFstBaseValue::ParseValue(type, stringValue, weight);
        if (!value.IsArray()) {
            return value;
        }
        const auto& array = value.GetArray();
        if (array.size() == 2 && value[0] == "-") {
            if (value[1].IsIntNumber()) {
                value = -value[1].GetIntNumber();
            } else if (value[1].IsNumber()) {
                value = -value[1].GetNumber();
            }
        }

        return value;
    }

} // namespace NAlice
