#include "is_knn_index_factor.h"

namespace NNlg {

TIsKnnIndexFactor::TIsKnnIndexFactor(const TString& knnIndexName)
    : KnnIndexName(knnIndexName)
{
}

void TIsKnnIndexFactor::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    for (size_t i = 0; i < factors->size(); ++i) {
        (*factors)[i].push_back(ctx.KnnIndexNames[i].StartsWith(KnnIndexName));
    }
}

}
