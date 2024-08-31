#pragma once

#include "fst_base_value.h"

namespace NAlice {

    class TFstFloat : public virtual TFstBaseValue {
    public:
        using TFstBaseValue::TFstBaseValue;

    protected:
        TParsedToken::TValueType TryParseNumber(TStringBuf str) const override;
    };

} // namespace NAlice
