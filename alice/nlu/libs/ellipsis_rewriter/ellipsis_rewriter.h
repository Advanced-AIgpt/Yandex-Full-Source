#pragma once

#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/rnn_tagger/rnn_tagger.h>

namespace NAlice {

struct TRewriterToken {
    TString Text;
    float NonsenseProbability = 0.f;
    bool IsMainRequestPart = false;
    bool IsRewrittenTextPart = false;
};

struct TEllipsisRewriterConfig {
    int MaxHistorySize = 7;
    float NonsenseProbabilityThreshold = 0.5f;

    TEllipsisRewriterConfig() = default;
    TEllipsisRewriterConfig(IInputStream* jsonInputStream);
};

class TEllipsisRewriter {
public:
    TEllipsisRewriter(const TVector<float>& separatorEmbedding,
                      const TTokenEmbedder& tokenEmbedder,
                      const TEllipsisRewriterConfig& config,
                      IInputStream* protobufModel,
                      IInputStream* modelDescription);

    TVector<TRewriterToken> Predict(const TVector<NVins::TSample>& dialogHistory) const;

private:
    NVins::TRnnTagger Tagger;
    TVector<float> SeparatorEmbedding;
    TTokenEmbedder TokenEmbedder;
    TEllipsisRewriterConfig Config;

private:
    NVins::TSampleFeatures PrepareSampleFeatures(const TVector<NVins::TSample>& dialogHistory) const;

    TVector<TRewriterToken> ConvertTaggerPrediction(const NVins::TPrediction& taggerPrediction,
                                                    const TVector<TString>& inputTokens,
                                                    const size_t mainRequestBeginIndex) const;
};

} // namespace NAlice
