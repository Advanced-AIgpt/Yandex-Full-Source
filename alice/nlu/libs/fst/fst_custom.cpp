#include "fst_custom.h"

#include <library/cpp/json/json_reader.h>

#include <util/string/strip.h>

namespace NAlice {

    TFstCustom::TFstCustom(const IDataLoader& loader)
        : TFstBase(loader), TFstMapMixin(loader)
    {
    }

    TParsedToken::TValueType TFstCustom::ParseValue(
        const TString& type
        , TString* stringValue
        , TMaybe<double>* weight) const
    {
        const auto normalized = StripAndCollapse(*stringValue);
        auto valueAndWeight = FindCanonicalValueAndWeight(normalized);
        if (!valueAndWeight) {
            return TFstBase::ParseValue(type, stringValue, weight);
        }
        *stringValue = std::move(normalized);
        *weight = valueAndWeight->Weight;

        return TStringBuf{valueAndWeight->Value};
    }

} // namespace NAlice
