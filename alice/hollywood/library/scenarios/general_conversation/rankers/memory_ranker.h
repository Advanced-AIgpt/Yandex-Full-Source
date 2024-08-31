#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/boltalka/memory/lstm_dssm/applier/dssm_applier.h>

#include <alice/megamind/protos/common/gc_memory_state.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TMemoryRanker {
public:
    struct TConfig {
        TString EmbedderName;
        bool IsPureGcRanker;
    };

    TMemoryRanker(IInputStream* model, IInputStream* modelConfig, IInputStream* rankerConfig);
    TMemoryRanker(IInputStream* model, IInputStream* modelConfig, const TConfig& rankerConfig);

    void Rerank(TVector<TAggregatedReplyCandidate>& candidates, const NAlice::TLstmState& modelProtoState, float rankerWeight, bool isPureGc) const;
    TString GetEmbedderName() const;

private:
    ::NNlg::TDSSMMemoryApplier DssmScorer;
    TString EmbedderName;
    bool IsPureGcRanker;
};

} // namespace NAlice::NHollywood::NGeneralConversation
