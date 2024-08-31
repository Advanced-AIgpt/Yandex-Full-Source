#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>

namespace NAlice::NHollywood::NGeneralConversation {

constexpr double COEF_RELEV_DEFAULT = 2.0;
constexpr double COEF_INFORMATIVENESS_DEFAULT = 100.0;
constexpr double COEF_SEQ2SEQ_DEFAULT = 0.0;
constexpr double COEF_INTEREST_DEFAULT = 0.8;
constexpr double COEF_NOT_RUDE_DEFAULT = 0.1;
constexpr double COEF_NOT_MALE_DEFAULT = 0.1;
constexpr double COEF_RESPECT_DEFAULT = 1.0;

TVector<TAggregatedReplyCandidate> RetireAggregatedReplyCandidatesResponse(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult);

} // namespace NAlice::NHollywood::NGeneralConversation
