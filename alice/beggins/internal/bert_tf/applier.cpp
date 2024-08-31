#include "applier.h"

#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>

using namespace NNeuralNet;

namespace NAlice::NBeggins::NBertTfApplier {

namespace {
TTfWorkerPtr MakeWorker(const TTfSessionPtr& session, const TVector<TString>& outputs) {
    return session->MakeWorker({"input_ids", "input_mask", "segment_ids"}, outputs);
}

TApplier::TResult GetWordEmbeddings(const TApplier::TResultForTokens& tokenEmbeddings,
                                    const TTokenizer::TResult& tokenization) {
    TApplier::TResult result{tokenEmbeddings, /* .WordEmbeddings = */ {}, /* .Words = */ {}, /*.Tokenization = */ tokenization};
    for (const auto tokenIdx : xrange(tokenization.Tokens.size())) {
        const auto& token = tokenization.Tokens[tokenIdx];
        if (token == CLS || token == SEP) {
            continue;
        }
        if (!tokenization.IsContinuation[tokenIdx]) {
            result.WordEmbeddings.push_back(tokenEmbeddings.TokenEmbeddings[tokenIdx]);
            result.Words.push_back(token);
        } else {
            for (const auto featureIdx : xrange(tokenEmbeddings.TokenEmbeddings[tokenIdx].size())) {
                result.WordEmbeddings.back()[featureIdx] += tokenEmbeddings.TokenEmbeddings[tokenIdx][featureIdx];
            }
            result.Words.back().append(token.substr(2, token.length() - 2));
        }
    }
    return result;
}
} // namespace

TApplier::TApplier(std::unique_ptr<TTokenizer> tokenizer, std::unique_ptr<TBertDict> dict,
                   const TTfGraphConfig& tfGraphConfig, const TTfSessionConfig& tfSessionConfig)
    : Tokenizer(std::move(tokenizer))
    , Preparer(std::make_unique<TBatchPreparer>(std::move(dict), tfGraphConfig.SequenceLength))
    , Outputs({tfGraphConfig.TokenEmbeddingsOutput, tfGraphConfig.SentenceEmbeddingOutput}) {
    Y_ENSURE(Tokenizer);
    GraphProcessor.Reset(TTfGraphProcessorBase::New(tfGraphConfig.GraphDefPath));
    Session.Reset(GraphProcessor->MakeSession(tfSessionConfig.NumInterOpThreads, tfSessionConfig.NumIntraOpThreads,
                                              tfSessionConfig.CudaDeviceIdx));
}

TVector<TApplier::TResultForTokens> TApplier::ApplyOnTokens(const TVector<TVector<TUtf32String>>& tokens) const {
    const auto input = Preparer->Prepare(tokens);

    const auto worker = MakeWorker(Session, Outputs);
    const auto resultTensors = worker->Process({input.InputIds, input.InputMask, input.SegmentIds});

    const auto tokenEmbeddingMap = resultTensors[0].template tensor<float, 3>();
    const auto sentenceEmbeddingMap = resultTensors[1].template tensor<float, 2>();

    const auto nSamples = tokens.size();
    const auto tokenEmbeddingDim = tokenEmbeddingMap.dimension(2);
    const auto sentenceEmbeddingDim = sentenceEmbeddingMap.dimension(1);

    TVector<TApplier::TResultForTokens> result(Reserve(nSamples));
    for (size_t sampleIdx : xrange(nSamples)) {
        const auto nTokens = tokens[sampleIdx].size();
        TResultForTokens currentResult{
            .TokenEmbeddings = TVector(nTokens, TEmbedding(tokenEmbeddingDim)),
            .SentenceEmbedding = TEmbedding(sentenceEmbeddingDim),
        };
        CopyN(&sentenceEmbeddingMap(sampleIdx, 0), sentenceEmbeddingDim, currentResult.SentenceEmbedding.begin());
        for (const auto tokenIdx : xrange(nTokens)) {
            CopyN(&tokenEmbeddingMap(sampleIdx, tokenIdx, 0), tokenEmbeddingDim,
                  currentResult.TokenEmbeddings[tokenIdx].begin());
        }
        result.push_back(std::move(currentResult));
    }
    return result;
}

TVector<TApplier::TResult> TApplier::Apply(const TVector<TUtf32String>& texts) const {
    const auto nSamples = texts.size();
    TVector<TVector<TUtf32String>> tokens(Reserve(nSamples));
    TVector<TTokenizer::TResult> tokenizations(Reserve(nSamples));
    for (const auto& text : texts) {
        tokenizations.push_back(Tokenizer->Tokenize(text));
        tokens.push_back(tokenizations.back().Tokens);
    }
    const auto resultForTokens = ApplyOnTokens(tokens);
    TVector<TResult> result(Reserve(nSamples));
    for (const auto sampleIdx : xrange(nSamples)) {
        result.push_back(GetWordEmbeddings(resultForTokens[sampleIdx], tokenizations[sampleIdx]));
    }
    return result;
}
} // namespace NAlice::NBeggins::NBertTfApplier
