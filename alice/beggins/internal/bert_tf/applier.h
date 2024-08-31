#pragma once

#include "literals.h"
#include "preparer.h"
#include "tokenizer.h"

#include <library/cpp/tf/graph_processor_base/graph_processor_base.h>

#include <util/generic/vector.h>

namespace NAlice::NBeggins::NBertTfApplier {

using TEmbedding = TVector<float>;

class TApplier {
public:
    struct TResultForTokens {
        TVector<TEmbedding> TokenEmbeddings;
        TEmbedding SentenceEmbedding;
    };

    struct TResult : TResultForTokens {
        TVector<TEmbedding> WordEmbeddings;
        TVector<TUtf32String> Words;
        TTokenizer::TResult Tokenization;
    };

    struct TTfGraphConfig {
        TString GraphDefPath;
        int SequenceLength;
        TString TokenEmbeddingsOutput;
        TString SentenceEmbeddingOutput;
    };

    struct TTfSessionConfig {
        int NumInterOpThreads;
        int NumIntraOpThreads;
        int CudaDeviceIdx = -1;
    };

    TApplier(std::unique_ptr<TTokenizer> tokenizer, std::unique_ptr<TBertDict> dict,
             const TTfGraphConfig& tfGraphConfig, const TTfSessionConfig& tfSessionConfig);
    TVector<TResult> Apply(const TVector<TUtf32String>& requests) const;

private:
    TVector<TResultForTokens> ApplyOnTokens(const TVector<TVector<TUtf32String>>& tokens) const;

    std::unique_ptr<TTokenizer> Tokenizer;
    std::unique_ptr<TBatchPreparer> Preparer;
    NNeuralNet::TTfSessionPtr Session;
    NNeuralNet::TTfGraphProcessorBasePtr GraphProcessor;
    TVector<TString> Outputs;
};
} // namespace NAlice::NBeggins::NBertTfApplier
