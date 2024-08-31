#include "string_with_weight.h"

namespace NGranet {

// ~~~~ TStringWithWeight functions ~~~~

void SortAndMerge(TVector<TStringWithWeight>* array) {
    Y_ENSURE(array);
    if (array->size() <= 1) {
        return;
    }
    Sort(*array);
    auto dst = array->begin();
    auto src = dst + 1;
    while (src != array->end()) {
        if (dst->String == src->String) {
            dst->Weight += src->Weight;
        } else {
            dst++;
            if (dst != src) {
                *dst = *src;
            }
        }
        src++;
    }
    dst++;
    array->erase(dst, array->end());
}

double CalculateWeightSum(const TVector<TStringWithWeight>& list) {
    double result = 0.;
    for (const TStringWithWeight& item : list) {
        result += item.Weight;
    }
    return result;
}

void MultiplyWeights(double coeff, TVector<TStringWithWeight>* list) {
    Y_ENSURE(list);
    for (TStringWithWeight& item : *list) {
        item.Weight *= coeff;
    }
}

void NormalizeWeights(TVector<TStringWithWeight>* list) {
    Y_ENSURE(list);
    const double srcSum = CalculateWeightSum(*list);
    if (srcSum != 0) {
        MultiplyWeights(1. / srcSum, list);
    }
}

} // namespace NGranet
