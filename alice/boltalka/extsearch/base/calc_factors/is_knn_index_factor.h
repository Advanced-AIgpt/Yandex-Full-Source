#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>

namespace NNlg {

class TIsKnnIndexFactor : public IFactor {
public:
    TIsKnnIndexFactor(const TString& knnIndexName);
    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return IS_KNN_INDEX_FACTOR_NAME_PREFIX + KnnIndexName;
    }

private:
    const TString KnnIndexName;
};

}
