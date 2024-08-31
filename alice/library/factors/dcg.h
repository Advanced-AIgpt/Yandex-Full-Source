#pragma once

#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <cmath>

namespace NAlice {

inline float CalcDCG(const ui32 position) {
    Y_ASSERT(position > 0);
    return 1.0f / (1.0f + std::log2(position)); // position is 1-based
}

inline float CalcDCGAt(const TVector<ui32>& positions, const ui32 at) {
    float dcg = 0.0f;
    for (const ui32 position : positions) {
        if (position <= at) {
            dcg += CalcDCG(position);
        }
    }
    return dcg;
}

} // namespace NAlice
