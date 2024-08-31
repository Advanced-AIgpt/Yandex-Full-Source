#include "fst_custom_hierarchy.h"

namespace NAlice {

    TFstCustomHierarchy::TFstCustomHierarchy(const IDataLoader& loader)
        : TFstCustom(loader)
    {
    }

    TParsedToken::TValueType TFstCustomHierarchy::ParseValue(
        const TString& type,
        TString* stringValue,
        TMaybe<double>* weight) const
    {
        const auto normalized = StripAndCollapse(*stringValue);
        if (type.Empty()) {
            return TFstCustom::ParseValue(type, stringValue, weight);
        }
        const auto searchValue = type + '_' + normalized;
        auto valueAndWeight = FindCanonicalValueAndWeight(searchValue);
        if (!valueAndWeight) {
            return TFstBase::ParseValue(type, stringValue, weight);
        }
        *stringValue = std::move(normalized);
        *weight = valueAndWeight->Weight;

        return TStringBuf{valueAndWeight->Value};
    }

} // namespace NAlice
