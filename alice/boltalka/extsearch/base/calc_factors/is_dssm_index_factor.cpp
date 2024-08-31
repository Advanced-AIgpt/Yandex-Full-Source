#include "is_dssm_index_factor.h"

namespace NNlg {

TIsDssmIndexFactor::TIsDssmIndexFactor(const TString& dssmIndexName)
    : DssmIndexName(dssmIndexName)
{
}

void TIsDssmIndexFactor::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    for (size_t i = 0; i < factors->size(); ++i) {
        (*factors)[i].push_back(ctx.DssmIndexNames[i] == DssmIndexName);
    }
}

}
