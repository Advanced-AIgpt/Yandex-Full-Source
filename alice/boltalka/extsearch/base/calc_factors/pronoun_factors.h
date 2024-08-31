#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <util/generic/fwd.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NNlg {

class TPronounFactors : public IFactor {
public:
    void AppendPronounFactors(TStringBuf text, TVector<float>* factors) const;

    void AppendQueryFactors(const TVector<TString>& queryTurns, TVector<TVector<float>>* factors) const;
    void AppendContextFactors(const TVector<TVector<TString>>& contexts, TVector<TVector<float>>* factors) const;
    void AppendReplyFactors(const TVector<TString>& replies, TVector<TVector<float>>* factors) const;

    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return PRONOUN_FACTORS_NAME;
    }

private:
    static const TVector<TString> Pronouns;
    static const THashMap<TString, size_t> PronounIds;
};

}
