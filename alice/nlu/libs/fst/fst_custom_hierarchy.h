#pragma once

#include "fst_custom.h"
#include "fst_map_mixin.h"

namespace NAlice {

    class TFstCustomHierarchy: public TFstCustom {
    public:
        explicit TFstCustomHierarchy(const IDataLoader& loader);
        TParsedToken::TValueType ParseValue(const TString& type, TString* stringValue, TMaybe<double>* weight) const override;
    };

} // namespace NAlice
