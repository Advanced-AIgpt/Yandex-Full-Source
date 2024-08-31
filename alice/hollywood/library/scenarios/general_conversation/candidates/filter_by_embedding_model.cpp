#include "filter_by_embedding_model.h"

#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/iterator/zip.h>

namespace NAlice::NHollywood::NGeneralConversation {

TFilterByEmbeddingModel::TFilterByEmbeddingModel(const NAlice::TBoltalkaDssmEmbedder& embedder, TStringBuf modelName, float border)
    : Embedder(embedder)
    , ModelName(modelName)
    , FilterBorder(border)
{
}

void TFilterByEmbeddingModel::FilterCandidates(TVector<TAggregatedReplyCandidate>& candidates, const TSessionState& sessionState) const {
    TVector<TString> usedReplies;
    usedReplies.reserve(sessionState.GetUsedRepliesInfo().size());
    for (const auto& usedReply : sessionState.GetUsedRepliesInfo()) {
        usedReplies.push_back(usedReply.GetText());
    }

    const auto& scores = ScoreCandidates(usedReplies, candidates);
    THashMap<TString, float> candidatesWithScores(candidates.size());
    for (const auto& [candidate, score] : Zip(candidates, scores)) {
        candidatesWithScores[GetAggregatedReplyText(candidate)] = score;
    }
    EraseIf(candidates, [&candidatesWithScores, this] (const auto& candidate) { return candidatesWithScores.Value(GetAggregatedReplyText(candidate), this->FilterBorder - 1.0) > this->FilterBorder;});
}

TVector<float> TFilterByEmbeddingModel::EmbedCandidate(const TAggregatedReplyCandidate& candidate) const {
    const auto precomputedEmbedding = candidate.GetEmbeddings().find(ModelName);
    if (precomputedEmbedding != candidate.GetEmbeddings().end()) {
        return {precomputedEmbedding->second.GetValue().begin(), precomputedEmbedding->second.GetValue().end()};
    }
    return Embedder.Embed(GetAggregatedReplyText(candidate));
}

TVector<float> TFilterByEmbeddingModel::ScoreCandidates(const TVector<TString>& usedReplies, const TVector<TAggregatedReplyCandidate>& candidates) const {
    TVector<TVector<float>> contextEmbeddings;
    contextEmbeddings.reserve(usedReplies.size());
    for (const auto& usedReply : usedReplies) {
        contextEmbeddings.emplace_back(Embedder.Embed(usedReply));
    }

    TVector<float> scores;
    scores.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        const auto candidateEmbedding = EmbedCandidate(candidate);
        float candidateScore = -1;
        for (const auto& contextEmbedding : contextEmbeddings) {
            candidateScore = Max(candidateScore, DotProduct(candidateEmbedding.data(), contextEmbedding.data(), contextEmbedding.size()));
        }
        scores.push_back(candidateScore);
    }
    return scores;
}

} // namespace NAlice::NHollywood::NGeneralConversation
