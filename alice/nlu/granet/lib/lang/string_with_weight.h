#pragma once

#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NGranet {

// ~~~~ TStringWithWeight ~~~~

struct TStringWithWeight {
    TString String;
    double Weight = 1.;

    DECLARE_TUPLE_LIKE_TYPE(TStringWithWeight, String, Weight);
};

// ~~~~ TStringWithWeight functions ~~~~

void SortAndMerge(TVector<TStringWithWeight>* list);

double CalculateWeightSum(const TVector<TStringWithWeight>& list);

void MultiplyWeights(double coeff, TVector<TStringWithWeight>* list);

void NormalizeWeights(TVector<TStringWithWeight>* list);

} // namespace NGranet

template <>
struct THash<NGranet::TStringWithWeight>: public TTupleLikeTypeHash {
};
