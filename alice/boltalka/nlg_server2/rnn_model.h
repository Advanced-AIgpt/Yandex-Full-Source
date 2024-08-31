#pragma once
#include "layers.h"

#include <util/random/fast.h>

namespace NNlgServer {

using TLstm = NDssm3::TLstm2<float>;

struct TComputationContext : public TThrRefBase {
    TFastRng<ui64> Rng = TFastRng<ui64>(0);
    TLstm Lstm;
    TMatrix LstmOutput;
    TMatrix LstmMemory;
    TMatrix SoftmaxProbs;
    TMatrix CurToken;
    TMatrix DecoderInput;
    TMatrix IntermediateFcOutput;
};
using TComputationContextPtr = TIntrusivePtr<TComputationContext>;

class TEncoderDecoder : public TThrRefBase {
public:
    TEncoderDecoder(const TFsPath &modelDir, bool lstmCorrectLayerOrder);

    TVector<ui64> GetSample(const TVector<ui64> &seed, ui64 maxLen, float temperature, TComputationContextPtr ctx) const;

private:
    // TODO(alipov): pass it parameter to constructor
    static const ui64 EOS = 0;
    // TODO(alipov): strip unnecessary tokens from OutputFc
    static const ui64 UNK = 1;

    bool LstmCorrectLayerOrder;
    TFcParametersPtr Embeddings;
    TLstm2ParametersPtr Encoder;
    TLstm2ParametersPtr Decoder;
    TFcParametersPtr IntermediateFc;
    TFcParametersPtr OutputFc;
};
using TEncoderDecoderPtr = TIntrusivePtr<TEncoderDecoder>;

}
