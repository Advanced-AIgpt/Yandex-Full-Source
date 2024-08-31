#pragma once

#include "fst_float.h"

namespace NAlice {

    class TFstUnits : public TFstFloat {
    public:
        using TFstFloat::TFstFloat;

    protected:
        TParsedToken::TValueType ProcessValue(const TParsedToken::TValueType& value) const override;
    };

} // namespace NAlice
