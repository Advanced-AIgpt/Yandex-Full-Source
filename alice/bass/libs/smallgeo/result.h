#pragma once

#include "region.h"

#include <util/generic/maybe.h>

#include <cstddef>

namespace NBASS {
namespace NSmallGeo {

struct TResult {
    static constexpr int MAX_SIMILARITY = 1000;

    TResult(size_t index, int similarity, int type, const TMaybe<double>& distanceM)
        : Index(index)
        , Similarity(similarity)
        , Type(type)
        , DistanceM(distanceM) {
    }

    size_t Index{};
    int Similarity{};
    int Type{};
    TMaybe<double> DistanceM{};
};

} // namespace NSmallGeo
} // namespace NBASS
