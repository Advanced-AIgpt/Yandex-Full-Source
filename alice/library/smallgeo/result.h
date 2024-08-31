#pragma once

#include "region.h"

#include <util/generic/maybe.h>

#include <cstddef>

namespace NAlice::NSmallGeo {

struct TResult {
    static constexpr int MaxSimilarity = 1000;

    TResult(size_t index, int similarity, int type, const TMaybe<double>& distanceM)
        : Index(index)
        , Similarity(similarity)
        , Type(type)
        , DistanceM(distanceM)
    {
    }

    size_t Index{};
    int Similarity{};
    int Type{};
    TMaybe<double> DistanceM{};
};

} // namespace NAlice::NSmallGeo
