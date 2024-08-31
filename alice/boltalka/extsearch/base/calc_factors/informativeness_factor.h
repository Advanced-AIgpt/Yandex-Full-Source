#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>

namespace NNlg {

class TInformativenessFactor : public IFactor {
public:
    TInformativenessFactor(size_t baseDssmQueryReplyCosineFactorIdx);
    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return INFORMATIVENESS_FACTOR_NAME;
    }

private:
    const size_t BaseDssmQueryReplyCosineFactorIdx;
};

}
