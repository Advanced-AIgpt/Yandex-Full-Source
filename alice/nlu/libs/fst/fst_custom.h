#pragma once

#include "fst_base.h"
#include "fst_map_mixin.h"

namespace NAlice {

    class TFstCustom: public TFstBase, protected TFstMapMixin {
    public:
        explicit TFstCustom(const IDataLoader& loader);
        TParsedToken::TValueType ParseValue(const TString& type, TString* stringValue, TMaybe<double>* weight) const override;
    };

} // namespace NAlice
