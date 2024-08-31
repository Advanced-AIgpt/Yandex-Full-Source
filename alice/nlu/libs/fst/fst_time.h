#pragma once

#include "fst_base_value.h"

namespace NAlice {

    class TFstTime : public TFstBaseValue {
    public:
        using TFstBaseValue::TFstBaseValue;

        TParsedToken::TValueType ProcessValue(const TParsedToken::TValueType& value) const override;
    };

} // namespace NAlice
