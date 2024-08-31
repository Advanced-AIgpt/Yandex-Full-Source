#pragma once

#include "fst_base_value.h"

namespace NAlice {

    class TFstNum : public virtual TFstBaseValue {
    public:
        using TFstBaseValue::TFstBaseValue;

    protected:
        TParsedToken::TValueType ParseValue(
            const TString& type, TString* stringValue, TMaybe<double>* weight) const override;
    };

} // namespace NAlice
