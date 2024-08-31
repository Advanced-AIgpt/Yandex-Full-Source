#include "memory_ranker.h"

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/json/json_reader.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

TMemoryRanker::TConfig LoadConfig(IInputStream* configStream) {
    Y_ENSURE(configStream);

    NJson::TJsonValue jsonConfig;
    const bool readCorrectly = NJson::ReadJsonTree(configStream, &jsonConfig);
    Y_ENSURE(readCorrectly);

    TMemoryRanker::TConfig config;
    config.EmbedderName = jsonConfig["embedder_name"].GetStringSafe();
    config.IsPureGcRanker = jsonConfig["is_pure_gc_ranker"].GetBooleanSafe();

    return config;
}

} // namespace

TMemoryRanker::TMemoryRanker(IInputStream* model, IInputStream* modelConfig, IInputStream* rankerConfig)
    : TMemoryRanker(model, modelConfig, LoadConfig(rankerConfig))
{
}

TMemoryRanker::TMemoryRanker(IInputStream* model, IInputStream* modelConfig, const TConfig& rankerConfig)
    : DssmScorer(model, modelConfig)
    , EmbedderName(rankerConfig.EmbedderName)
    , IsPureGcRanker(rankerConfig.IsPureGcRanker)
{
    DssmScorer.Predict(TVector<float>(DssmScorer.GetStateSize()), TVector<TVector<float>>(1, TVector<float>(DssmScorer.GetEmbeddingSize())));
}

void TMemoryRanker::Rerank(TVector<TAggregatedReplyCandidate>& candidates, const NAlice::TLstmState& modelProtoState, float rankerWeight, bool isPugeGc) const {
    if (IsPureGcRanker && !isPugeGc) {
        return;
    }

    TVector<float> modelState(modelProtoState.GetHVector().begin(), modelProtoState.GetHVector().end());

    TVector<TVector<float>> replyEmbeddings;
    replyEmbeddings.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        const auto embeddingIterator = candidate.GetEmbeddings().find(EmbedderName);
        Y_ASSERT(embeddingIterator != candidate.GetEmbeddings().end());
        replyEmbeddings.emplace_back(embeddingIterator->second.GetValue().begin(), embeddingIterator->second.GetValue().end());
    }

    const auto scores = DssmScorer.Predict(modelState, replyEmbeddings);
    for (auto [i, candidate] : Enumerate(candidates)) {
        auto relevance = candidate.GetRelevance();
        if (relevance > 0) {
            relevance = log(relevance);
        }
        relevance += rankerWeight * scores[i];
        candidate.SetRelevance(relevance);
    }
    SortBy(candidates, [](const auto& candidate) {return -candidate.GetRelevance();});
}

TString TMemoryRanker::GetEmbedderName() const {
    return EmbedderName;
}

} // namespace NAlice::NHollywood::NGeneralConversation
