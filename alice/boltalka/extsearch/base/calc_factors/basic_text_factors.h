#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>

namespace NNlg {

class TBasicTextFactors : public IFactor {
public:
    void AppendPunctFactors(TStringBuf text, TVector<float>* factors, const TString& punctuation = "?!.,()") const;
    void AppendWordLengthFactors(TStringBuf text, TVector<float>* factors) const;
    void AppendQueryFactors(const TVector<TString>& queryTurns, TVector<TVector<float>>* factors) const;
    void AppendContextFactors(const TVector<TVector<TString>>& contexts, TVector<TVector<float>>* factors) const;
    void AppendReplyFactors(const TVector<TString>& replies, TVector<TVector<float>>* factors) const;

    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return BASIC_TEXT_FACTORS_NAME;
    }
};

}
