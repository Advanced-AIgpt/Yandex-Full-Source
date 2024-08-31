#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>

namespace NNlg {

class TIsDssmIndexFactor : public IFactor {
public:
    TIsDssmIndexFactor(const TString& dssmIndexName);
    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return IS_DSSM_INDEX_FACTOR_NAME_PREFIX + DssmIndexName;
    }

private:
    const TString DssmIndexName;
};

}
