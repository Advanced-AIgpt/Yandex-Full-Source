#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

namespace NNlg {

class TSeq2SeqFactor : public IFactor {
public:
    TSeq2SeqFactor();
    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return SEQ2SEQ_FACTOR_NAME;
    }
};

}
