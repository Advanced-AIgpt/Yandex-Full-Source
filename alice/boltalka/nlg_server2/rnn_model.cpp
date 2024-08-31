#include "rnn_model.h"

#include <util/generic/algorithm.h>

namespace NNlgServer {

namespace {

template<class TVectorType>
static ui64 SampleFromProbsVec(TFastRng<ui64> &rng, TVectorType &probs, float temperature) {
    double sum = 0.0;
    for (auto &p : probs) {
        sum += std::pow(p, 1.0 / temperature);
        p = sum;
    }
    return UpperBound(probs.begin(), probs.end(), rng.GenRandReal4() * sum) - probs.begin();
}

}

TEncoderDecoder::TEncoderDecoder(const TFsPath &modelDir, bool lstmCorrectLayerOrder)
    : LstmCorrectLayerOrder(lstmCorrectLayerOrder)
    , Embeddings(LoadFc(modelDir, "context_embedding", false))
    , Encoder(LoadLstm2(modelDir, "context_lstm"))
    , Decoder(LoadLstm2(modelDir, "reply_lstm"))
    , IntermediateFc(LoadFc(modelDir, "fc_intermediate_decoder"))
    , OutputFc(LoadFc(modelDir, "fc_decoder"))
{
    for (size_t i = 0; i < Embeddings->Weights->GetNumRows(); ++i) {
        Embeddings->Weights->Add(i, (*Embeddings->Biases)[0], 1.f);
    }
}

TVector<ui64> TEncoderDecoder::GetSample(const TVector<ui64> &seed, ui64 maxLen, float temperature, TComputationContextPtr ctx) const {
    ctx->LstmOutput.Resize(1, Encoder->GetMemorySize(), 0.0);
    ctx->LstmMemory.Resize(1, Encoder->GetMemorySize(), 0.0);
    ctx->CurToken.Resize(1, Embeddings->Weights->GetNumColumns());
    for (ui64 idx : seed) {
        ctx->CurToken.Assign((*Embeddings->Weights)[idx]);
        ctx->Lstm.Fprop(*Encoder, ctx->CurToken, ctx->LstmMemory, ctx->LstmOutput, &ctx->LstmMemory, &ctx->LstmOutput, LstmCorrectLayerOrder);
    }

    ctx->DecoderInput.Resize(1, Embeddings->Weights->GetNumColumns() + Encoder->GetMemorySize());
    NDssm3::TVectorView<float> curTokenPart(ctx->DecoderInput.data(), Embeddings->Weights->GetNumColumns());
    NDssm3::TVectorView<float> encoderOutputPart(ctx->DecoderInput.data() + Embeddings->Weights->GetNumColumns(), Encoder->GetMemorySize());
    curTokenPart.Assign((*Embeddings->Biases)[0]);
    encoderOutputPart.Assign(ctx->LstmOutput[0]);
    ctx->LstmOutput.Resize(1, Decoder->GetMemorySize(), 0.0);
    ctx->LstmMemory.Resize(1, Decoder->GetMemorySize(), 0.0);
    ctx->SoftmaxProbs.Resize(1, OutputFc->Weights->GetNumRows(), 0.0);
    ctx->IntermediateFcOutput.Resize(1, IntermediateFc->Weights->GetNumRows());

    TVector<ui64> sample;
    for (ui64 i = 0; i < maxLen; ++i) {
        ctx->Lstm.Fprop(*Decoder, ctx->DecoderInput, ctx->LstmMemory, ctx->LstmOutput, &ctx->LstmMemory, &ctx->LstmOutput, LstmCorrectLayerOrder);
        NDssm3::TBatchAffineTransform::Fprop(ctx->LstmOutput, *IntermediateFc->Weights, (*IntermediateFc->Biases)[0], &ctx->IntermediateFcOutput);
        NDssm3::TBatchElemwiseTransform<NDssm3::TTanh>::Fprop(ctx->IntermediateFcOutput, &ctx->IntermediateFcOutput);
        NDssm3::TBatchAffineTransform::Fprop(ctx->IntermediateFcOutput, *OutputFc->Weights, (*OutputFc->Biases)[0], &ctx->SoftmaxProbs);
        NDssm3::TSoftmax::Fprop(ctx->SoftmaxProbs[0], &ctx->SoftmaxProbs[0]);

        float unkProb = ctx->SoftmaxProbs[0][UNK];
        ctx->SoftmaxProbs[0][UNK] = 0.0;
        NDssm3::TScale::Fprop(ctx->SoftmaxProbs[0], 1.0 / (1.0 - unkProb), 0.0, &ctx->SoftmaxProbs[0]);

        ui64 idx = SampleFromProbsVec(ctx->Rng, ctx->SoftmaxProbs[0], temperature);
        curTokenPart.Assign((*Embeddings->Weights)[idx]);
        sample.push_back(idx);
        if (idx == EOS) {
            break;
        }
    }
    return sample;
}

}
