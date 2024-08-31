#include "informativeness_factor.h"
#include <alice/boltalka/extsearch/base/factors/factors_gen.h>

#include <util/generic/ymath.h>

namespace NNlg {

TInformativenessFactor::TInformativenessFactor(size_t baseDssmQueryReplyCosineFactorIdx)
    : BaseDssmQueryReplyCosineFactorIdx(baseDssmQueryReplyCosineFactorIdx)
{
}

void TInformativenessFactor::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    for (size_t i = 0; i < factors->size(); ++i) {
        float numerator = exp((*factors)[i][ctx.NumStaticFactors + BaseDssmQueryReplyCosineFactorIdx]) * 1000;
        float denominator = FI_INFORMATIVENESS_DENOMINATOR < ctx.NumStaticFactors ? ctx.StaticFactors[i][FI_INFORMATIVENESS_DENOMINATOR] : 0;
        if (denominator == 0) {
            (*factors)[i].push_back(0);
        } else {
            (*factors)[i].push_back(numerator / denominator);
        }
    }
}

}
