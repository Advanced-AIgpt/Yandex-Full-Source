#pragma once
#include "factor.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <util/generic/fwd.h>

namespace NNlg {

class TRusListerFactors : public IFactor {
public:
    TRusListerFactors(const TString& filename);
    TRusListerFactors(const TVector<TString>& fileLines);

    void AppendRusListerFactors(TStringBuf text, TVector<float>* factors) const;

    void AppendQueryFactors(const TVector<TString>& queryTurns, TVector<TVector<float>>* factors) const;
    void AppendContextFactors(const TVector<TVector<TString>>& contexts, TVector<TVector<float>>* factors) const;
    void AppendReplyFactors(const TVector<TString>& replies, TVector<TVector<float>>* factors) const;

    void AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const override;

    TString GetName() const override {
        return RUS_LISTER_FACTORS_NAME;
    }

private:
    void AddFileLine(TStringBuf line);

private:
    static const ui8 TagBitCount;
    static const ui64 TagMarkerBit;
    static const ui64 TagMask;
    TDenseHash<ui64, ui64> WordTags;
    size_t NumTags;
};

}
