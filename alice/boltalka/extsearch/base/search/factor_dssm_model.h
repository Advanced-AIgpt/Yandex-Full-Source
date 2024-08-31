#pragma once

#include <alice/boltalka/extsearch/base/util/memory_mode.h>
#include <alice/boltalka/libs/dssm_model/dssm_pool.h>

namespace NNlg {

class TFactorDssmModel : public TThrRefBase {
public:
    TFactorDssmModel(const TString& dssmModelName, size_t numSearchThreads, EMemoryMode memoryMode);

    TVector<float> Fprop(const TVector<TString>& context);

    const float* GetContextReplyEmbedding(size_t docId) const;
    const float* GetZeroEmbedding() const;
    size_t GetDimension() const;

private:
    TDssmPoolPtr DssmPool;
    TBlob ContextReplyEmbeddings;
    size_t Dimension;
    const TVector<float> ZeroEmbedding;
};

using TFactorDssmModelPtr = TIntrusivePtr<TFactorDssmModel>;

}

