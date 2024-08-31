#include "aggregated_reply_wrapper.h"

#include "consts.h"

#include <alice/boltalka/libs/factors/proto/nlgsearch_factors.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

TString GetAggregatedReplyText(const TAggregatedReplyCandidate& reply) {
    switch (reply.GetReplySourceCase()) {
        case TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            return reply.GetNlgSearchReply().GetText();
        case TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            return reply.GetSeq2SeqReply().GetText();
        case TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            Y_ENSURE(false);
    }
}

TString GetAggregatedReplySource(const TAggregatedReplyCandidate& reply) {
    switch (reply.GetReplySourceCase()) {
        case TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            return reply.GetNlgSearchReply().GetSource();
        case TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            return TString{SOURCE_SEQ2SEQ};
        case TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            Y_ENSURE(false);
    }
}

double GetAggregatedReplyRelevance(const TAggregatedReplyCandidate& reply) {
    switch (reply.GetReplySourceCase()) {
        case TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            return reply.GetNlgSearchReply().GetRelevance();
        case TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            return reply.GetSeq2SeqReply().GetRelevance();
        case TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            Y_ENSURE(false);
    }
}

double GetAggregatedReplyInformativeness(const TAggregatedReplyCandidate& reply) {
    switch (reply.GetReplySourceCase()) {
        case TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            return reply.GetNlgSearchReply().GetFactors().GetInformativeness();
        case TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            return 0;
        case TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            Y_ENSURE(false);
    }
}

double GetAggregatedReplyDssmScore(const TAggregatedReplyCandidate& reply) {
    switch (reply.GetReplySourceCase()) {
        case TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            return reply.GetNlgSearchReply().GetFactors().GetDssmScore();
        case TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            return 0;
        case TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            Y_ENSURE(false);
    }
}

double GetAggregatedReplySeq2Seq(const TAggregatedReplyCandidate& reply) {
    switch (reply.GetReplySourceCase()) {
        case TAggregatedReplyCandidate::ReplySourceCase::kNlgSearchReply:
            return reply.GetNlgSearchReply().GetFactors().GetSeq2Seq();
        case TAggregatedReplyCandidate::ReplySourceCase::kSeq2SeqReply:
            return 1;
        case TAggregatedReplyCandidate::ReplySourceCase::REPLYSOURCE_NOT_SET:
            Y_ENSURE(false);
    }
}

} // namespace NAlice::NHollywood::NGeneralConversation
