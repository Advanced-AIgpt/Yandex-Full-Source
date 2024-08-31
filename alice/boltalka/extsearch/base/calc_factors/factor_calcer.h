#pragma once
#include "factor.h"
#include "factor_calcer_ctx.h"

namespace NNlg {

class TFactorCalcer : public TThrRefBase {
public:
    TFactorCalcer(TVector<IFactorPtr> factors, size_t numTurns);
    void CalcFactors(TFactorCalcerCtx* ctx, TVector<TVector<float>>* factors) const;
    const std::pair<size_t, size_t>* GetFactorsLocation(const TString& factorName) const;

private:
    const TVector<IFactorPtr> Factors;
    const size_t NumTurns;
    const THashMap<TString, std::pair<size_t, size_t>> FactorsLocation;
    const size_t NumFactors;
};

using TFactorCalcerPtr = TIntrusivePtr<TFactorCalcer>;

}
