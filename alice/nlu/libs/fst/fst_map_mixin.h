#pragma once

#include "fst_base.h"

#include <util/generic/maybe.h>
#include <util/generic/hash.h>

namespace NAlice {
    struct TValueAndWeight {
        TString Value;
        TMaybe<double> Weight;
    };

    class TFstMapMixin {
    public:
        explicit TFstMapMixin(const IDataLoader& loader);

        void LoadMapAndWeights(IInputStream& map, IInputStream* weights);
        TMaybe<TValueAndWeight> FindCanonicalValueAndWeight(const TString& value) const;

    private:
        THashMap<TString, TValueAndWeight> CanonicalValuesAndWeights;
    };

} // namespace NAlice
