#include "dssm_cos_factors.h"

#include <library/cpp/dot_product/dot_product.h>

namespace NNlg {

TDssmCosFactors::TDssmCosFactors(const TString& dssmModelName)
    : DssmModelName(dssmModelName)
{
}

const TString& TDssmCosFactors::GetDssmModelName() const {
    return DssmModelName;
}

void TDssmCosFactors::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    Y_ENSURE(ctx.DssmFactorCtxs.contains(DssmModelName));
    const auto& dssmCtx = ctx.DssmFactorCtxs.find(DssmModelName)->second;
    for (size_t doc = 0; doc < factors->size(); ++doc) {
        auto& docFactors = (*factors)[doc];
        docFactors.push_back(DotProduct(dssmCtx.ContextEmbeddings[doc], dssmCtx.ReplyEmbeddings[doc], dssmCtx.Dimension));
        docFactors.push_back(DotProduct(dssmCtx.QueryEmbedding, dssmCtx.ContextEmbeddings[doc], dssmCtx.Dimension));
        docFactors.push_back(DotProduct(dssmCtx.QueryEmbedding, dssmCtx.ReplyEmbeddings[doc], dssmCtx.Dimension));
        docFactors.push_back(docFactors[docFactors.size() - 2] + docFactors.back());
    }
}

}

