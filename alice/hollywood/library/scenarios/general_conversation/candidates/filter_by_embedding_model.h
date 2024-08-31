#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/nlu/libs/binary_classifier/boltalka_dssm_embedder.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TFilterByEmbeddingModel {
public:
    TFilterByEmbeddingModel(const NAlice::TBoltalkaDssmEmbedder& embedder, TStringBuf modelName, float border);

    void FilterCandidates(TVector<TAggregatedReplyCandidate>& candidates, const TSessionState& sessionState) const;

private:
    TVector<float> ScoreCandidates(const TVector<TString>& usedReplies, const TVector<TAggregatedReplyCandidate>& candidates) const;
    TVector<float> EmbedCandidate(const TAggregatedReplyCandidate& candidate) const;

private:
    const NAlice::TBoltalkaDssmEmbedder& Embedder;
    const TString ModelName;
    const float FilterBorder;
};

} // namespace NAlice::NHollywood::NGeneralConversation
