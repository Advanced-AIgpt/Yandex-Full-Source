#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>

namespace NNlg {

class TTextIntersectionFactors : public IFactor {
public:
    static THashMap<TStringBuf, ui32> GetWordFrequencies(TStringBuf text);
    static THashMap<TStringBuf, ui32> GetTrigramFrequencies(TStringBuf text);
    static ui32 CalcIntersection(const THashMap<TStringBuf, ui32>& a, const THashMap<TStringBuf, ui32>& b);
    static float CalcIntersectionFraction(const THashMap<TStringBuf, ui32>& a, const THashMap<TStringBuf, ui32>& b);

    void AppendWordIntersectionFactors(const TVector<TString>& queryTurns, const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, TVector<TVector<float>>* factors) const;
    void AppendTrigramIntersectionFactors(const TVector<TString>& queryTurns, const TVector<TVector<TString>>& contexts, const TVector<TString>& replies, TVector<TVector<float>>* factors) const;

    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return TEXT_INTERSECTION_FACTORS_NAME;
    }
};

}
