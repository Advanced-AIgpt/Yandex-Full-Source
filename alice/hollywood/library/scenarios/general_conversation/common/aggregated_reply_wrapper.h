#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <util/generic/fwd.h>
#include <util/stream/output.h>

namespace NAlice::NHollywood::NGeneralConversation {

TString GetAggregatedReplyText(const TAggregatedReplyCandidate& replyCandidate);
TString GetAggregatedReplySource(const TAggregatedReplyCandidate& replyCandidate);
double GetAggregatedReplyRelevance(const TAggregatedReplyCandidate& reply);
double GetAggregatedReplyInformativeness(const TAggregatedReplyCandidate& reply);
double GetAggregatedReplyDssmScore(const TAggregatedReplyCandidate& reply);
double GetAggregatedReplySeq2Seq(const TAggregatedReplyCandidate& reply);

} // namespace NAlice::NHollywood::NGeneralConversation

template <>
inline void Out<NAlice::NHollywood::NGeneralConversation::TAggregatedReplyCandidate::ReplySourceCase>(
    IOutputStream& out, NAlice::NHollywood::NGeneralConversation::TAggregatedReplyCandidate::ReplySourceCase value)
{
    switch (value) {
        case NAlice::NHollywood::NGeneralConversation::TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            out << "NlgSearchReply";
            break;
        case NAlice::NHollywood::NGeneralConversation::TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            out << "Seq2SeqReply";
            break;
        case NAlice::NHollywood::NGeneralConversation::TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            out << "REPLYSOURCE_NOT_SET";
            break;
    }
}
