#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>

namespace NNlg {

const size_t DSSM_QUERY_REPLY_COSINE_FACTOR_SHIFT = 2;

class TDssmCosFactors : public IFactor {
public:
    TDssmCosFactors(const TString& dssmModelName);
    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    const TString& GetDssmModelName() const;

    TString GetName() const override {
        return DSSM_COS_FACTORS_NAME_PREFIX + DssmModelName;
    }

private:
    const TString DssmModelName;
};

}
