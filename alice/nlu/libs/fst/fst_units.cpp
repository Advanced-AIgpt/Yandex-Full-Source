#include "fst_units.h"

namespace NAlice {

    TParsedToken::TValueType TFstUnits::ProcessValue(const TParsedToken::TValueType& value) const
    {
        if (!value.IsArray()) {
            return value;
        }

        NSc::TValue out;
        for (size_t i = 0u; i < value.ArraySize(); i += 2) {
            out.Add(value[i+1].GetString()) = value[i];
        }

        return out;
    }

} // namespace NAlice
