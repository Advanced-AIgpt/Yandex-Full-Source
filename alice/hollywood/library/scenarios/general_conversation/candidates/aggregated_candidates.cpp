#include "aggregated_candidates.h"

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/bert_reranker.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/phead_scorer.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/seq2seq_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/boltalka/generative/service/proto/bert_response.pb.h>

#include <alice/library/logger/logger.h>

#include <alice/protos/data/language/language.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

double CalculateRelevance(const TAggregatedReplyCandidate& candidate, TGeneralConversationRunContextWrapper& contextWrapper) {
    double score = 0.;
    const auto& testId = NAlice::GetExperimentValueWithPrefix(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_OPTIMIZATION_SAMPLE);
    auto fastData = contextWrapper.FastData();
    const auto& bertOutput = candidate.GetBertOutput();
    TString date = GetCurrentDate(contextWrapper);
    if(!testId || !((fastData->GetCombinationCoefficientsDict().count(date) == 1) && (fastData->GetCombinationCoefficientsDict().at(date).count(FromString<int32_t>(testId.GetRef())) == 1))) {
        const auto bertRelevCoef = GetExperimentTypedValue<double>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_BERT_RERANKER_COEF_RELEV);
        score += bertRelevCoef.GetOrElse(COEF_RELEV_DEFAULT) * bertOutput.GetRelevScore();
        const auto bertInformativenessCoef = GetExperimentTypedValue<double>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_BERT_RERANKER_COEF_INFORMATIVENESS);
        score += bertInformativenessCoef.GetOrElse(COEF_INFORMATIVENESS_DEFAULT) * bertOutput.GetInformativenessScore();
        const auto bertInterestCoef = GetExperimentTypedValue<double>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_BERT_RERANKER_COEF_INTEREST);
        score += bertInterestCoef.GetOrElse(COEF_INTEREST_DEFAULT) * bertOutput.GetInterestScore();
        const auto bertNotRudeCoef = GetExperimentTypedValue<double>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_BERT_RERANKER_COEF_NOT_RUDE);
        score += bertNotRudeCoef.GetOrElse(COEF_NOT_RUDE_DEFAULT) * bertOutput.GetNotRudeScore();
        const auto bertNotMaleCoef = GetExperimentTypedValue<double>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_BERT_RERANKER_COEF_NOT_MALE);
        score += bertNotMaleCoef.GetOrElse(COEF_NOT_MALE_DEFAULT) * bertOutput.GetNotMaleScore();
        const auto bertRespectCoef = GetExperimentTypedValue<double>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_BERT_RERANKER_COEF_RESPECT);
        score += bertRespectCoef.GetOrElse(COEF_RESPECT_DEFAULT) * bertOutput.GetRespectScore();
        return score;
    }
    auto currentLinearCoefficients = fastData->GetCombinationCoefficientsDict().at(date).at(FromString<int32_t>(testId.GetRef()));
    score += currentLinearCoefficients.Relev * bertOutput.GetRelevScore();
    score += currentLinearCoefficients.Informativeness * bertOutput.GetInformativenessScore();
    score += currentLinearCoefficients.Interest * bertOutput.GetInterestScore();
    score += currentLinearCoefficients.NotRude * bertOutput.GetNotRudeScore();
    score += currentLinearCoefficients.NotMale * bertOutput.GetNotMaleScore();
    score += currentLinearCoefficients.Respect * bertOutput.GetRespectScore();
    return score;
}

bool GetAggregatedReplies(TGeneralConversationRunContextWrapper& contextWrapper, TVector<TAggregatedReplyCandidate>* result) {
    auto aggregatedResponseMaybe = GetMaybeOnlyProto<TAggregatedRepliesState>(contextWrapper.Ctx()->ServiceCtx, STATE_AGGREGATED_REPLIES);
    if (!aggregatedResponseMaybe) {
        return false;
    }
    result->reserve(aggregatedResponseMaybe->GetReplyCandidates().size());
    for (auto candidate : aggregatedResponseMaybe->GetReplyCandidates()) {
        result->push_back(std::move(candidate));
    }
    if (RetireBertRerankerResponse(*contextWrapper.Ctx(), result)) {
        for (auto& candidate : *result) {
            candidate.SetRelevance(CalculateRelevance(candidate, contextWrapper));
        }
    } else if (!result->empty()) {
        const auto sourceCase = result->front().GetReplySourceCase();
        LOG_INFO(contextWrapper.Ctx()->Ctx.Logger()) << "No results from bert, leave only: " << ToString(sourceCase);
        EraseIf(*result, [&sourceCase] (const auto& candidate) { return candidate.GetReplySourceCase() != sourceCase; });
        for (auto& candidate : *result) {
            candidate.SetRelevance(GetAggregatedReplyRelevance(candidate));
        }
    }

    SortBy(*result, [](const auto& candidate) {return -candidate.GetRelevance();});
    result->crop(MAX_CANDIDATES_SIZE);

    return true;
}

void SelectSeq2SeqReplyByScore(TVector<TSeq2SeqReplyCandidate>& seq2SeqCandidates) {
    if (seq2SeqCandidates.size() <= 1) {
        return;
    }
    size_t maxScoreIdx = 0;
    float maxScore = seq2SeqCandidates[maxScoreIdx].GetRelevance();
    for (size_t i = 1; i < seq2SeqCandidates.size(); ++i) {
        if (seq2SeqCandidates[i].GetRelevance() > maxScore) {
            maxScore = seq2SeqCandidates[i].GetRelevance();
            maxScoreIdx = i;
        }
    }
    seq2SeqCandidates = { seq2SeqCandidates[maxScoreIdx] };
}

bool IsArabicQueryBad(TGeneralConversationRunContextWrapper& contextWrapper) {
    auto pheadResponse = RetirePheadResponse(*contextWrapper.Ctx());
    if (!pheadResponse.Defined()) {
        return false;
    }
    const auto& pheadScores = pheadResponse.Get()->Scores;
    if (pheadScores.size() != 1) {
        return false;
    }
    const auto pheadScore = pheadScores[0];
    const auto banClfThreshold = GetExperimentTypedValue<float>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_ARABOBA_BAN_CLF_THRESHOLD_PREFIX).GetOrElse(DEFAULT_ARABOBA_BAN_CLF_THRESHOLD);
    return pheadScore > banClfThreshold;
}

}

TVector<TAggregatedReplyCandidate> RetireAggregatedReplyCandidatesResponse(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult) {
    TVector<TAggregatedReplyCandidate> result;
    if (GetAggregatedReplies(contextWrapper, &result)) {
        return result;
    }

    if (classificationResult.GetHasSeq2SeqReplyRequest()) {
        if (classificationResult.GetUserLanguage() == ELang::L_ARA && IsArabicQueryBad(contextWrapper)) {
            return result;
        }
        if (auto seq2seqResponse = RetireReplySeq2SeqCandidatesResponse(*contextWrapper.Ctx()); seq2seqResponse.Defined()) {
            auto& seq2seqCandidates = seq2seqResponse.GetRef();
            const bool enableSampleAndRank = classificationResult.GetUserLanguage() == ELang::L_ARA && !contextWrapper.RequestWrapper().HasExpFlag(EXP_HW_GC_DISABLE_SAMPLE_AND_RANK);
            if (enableSampleAndRank) {
                SelectSeq2SeqReplyByScore(seq2seqCandidates);
            }
            for (auto& candidate : seq2seqCandidates) {
                TAggregatedReplyCandidate aggregatedCandidate;
                *aggregatedCandidate.MutableSeq2SeqReply() = std::move(candidate);
                result.push_back(std::move(aggregatedCandidate));
            }
        }
    } else if (classificationResult.GetHasSearchReplyRequest()) {
        const bool preferChildReply = !contextWrapper.RequestWrapper().HasExpFlag(EXP_HW_GC_DISABLE_CHILD_REPLIES) && classificationResult.GetIsChildTalking();
        for (auto& candidate : RetireReplyCandidatesResponse(*contextWrapper.Ctx(), preferChildReply)) {
            TAggregatedReplyCandidate aggregatedCandidate;
            aggregatedCandidate.SetRelevance(candidate.GetRelevance());
            *aggregatedCandidate.MutableNlgSearchReply() = std::move(candidate);
            result.push_back(std::move(aggregatedCandidate));
        }
    }
    return result;
}

} // namespace NAlice::NHollywood::NGeneralConversation
