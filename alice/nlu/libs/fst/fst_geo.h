#pragma once

#include "fst_base_value.h"
#include "fst_map_mixin.h"

namespace NAlice {

    class TFstGeo : public TFstBaseValue, private TFstMapMixin {
    public:
        explicit TFstGeo(const IDataLoader& loader)
            : TFstBaseValue(loader)
            , TFstMapMixin(loader)
        {
        }

        TParsedToken::TValueType ProcessValue(const TParsedToken::TValueType& value) const override;
    };

} // namespace NAlice
