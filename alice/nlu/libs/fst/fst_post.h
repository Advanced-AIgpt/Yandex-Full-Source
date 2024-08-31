#pragma once

#include "fst_base.h"

#include <util/generic/vector.h>

namespace NAlice {

    class TFstPost {
    public:
        static void CombineEntities(TVector<TEntity>* entities);
    };

} // namespace NAlice
