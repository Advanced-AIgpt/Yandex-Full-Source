#pragma once

#include "fst_float.h"
#include "fst_num.h"

namespace NAlice {

    class TFstCalc : public TFstFloat, public TFstNum {
    public:
        explicit TFstCalc(const IDataLoader& loader)
            : TFstBaseValue(loader),
              TFstFloat(loader),
              TFstNum(loader)
        {
        }

    protected:
        TParsedToken::TValueType ParseValue(
            const TString& type, TString* stringValue, TMaybe<double>* weight) const override;
    };

} // namespace NAlice
