#include "factor_dssm_model.h"

#include <kernel/searchlog/errorlog.h>

namespace NNlg {

TFactorDssmModel::TFactorDssmModel(const TString& dssmModelName, size_t numSearchThreads, EMemoryMode memoryMode)
    : DssmPool(new TDssmPool(dssmModelName, numSearchThreads))
    , ContextReplyEmbeddings(memoryMode == EMemoryMode::Locked ? TBlob::LockedFromFile(dssmModelName + ".vec") : TBlob::PrechargedFromFile(dssmModelName + ".vec"))
    , Dimension(DssmPool->Fprop({ "" }, "", { "query_embedding" })[0].size()) // TODO(alipov): looks kinda hacky
    , ZeroEmbedding(Dimension * 2)
{
    SEARCH_INFO << dssmModelName << " embedding dimension is " << Dimension << Endl;
}

const float* TFactorDssmModel::GetContextReplyEmbedding(size_t docId) const {
    return reinterpret_cast<const float*>(ContextReplyEmbeddings.AsCharPtr()) + docId * Dimension * 2;
}

const float* TFactorDssmModel::GetZeroEmbedding() const {
    return ZeroEmbedding.data();
}

size_t TFactorDssmModel::GetDimension() const {
    return Dimension;
}

TVector<float> TFactorDssmModel::Fprop(const TVector<TString>& context) {
    return DssmPool->Fprop(context, "", { "query_embedding" })[0];
}

}

