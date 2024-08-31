#include "fst_calc.h"

#include <util/string/strip.h>

namespace NAlice {

    TParsedToken::TValueType TFstCalc::ParseValue(
        const TString& /*type*/, TString* stringValue, TMaybe<double>* /*weight*/) const
    {
        TParsedToken::TValueType value = TStringBuf{*stringValue};
        StripInPlace(*stringValue);
        return value;
    }

} // namespace NAlice
