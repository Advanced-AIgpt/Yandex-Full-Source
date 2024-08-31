#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <util/generic/string.h>

namespace NAlice::NHollywood::NGeneralConversation {

TString GetStageName(const TGenerativeTaleState::EStage& stage);
TGenerativeTaleState::EStage MoveToPreviousQuestion(const TGenerativeTaleState::EStage& stage);
TGenerativeTaleState::EStage MoveToNextQuestion(const TGenerativeTaleState::EStage& stage);
void ParseGenerativeTaleQuestion(const TVector<TSeq2SeqReplyCandidate>& seq2seqResponse, TReplyInfo& replyInfo);
TString TaleAddQuestion(const TString& question, const TString& answer, const TString& prefix);

TString TaleDropEmptyStart(TString text);
TString TaleDropEnd(TString text);

template <typename TContextWrapper>
bool NeedTalesOnboarding(const TContextWrapper& contextWrapper, const TGenerativeTaleState& taleState);

TString MakeSharedLinkImageUrl(const TString& avatarsId, bool full = false);

} // namespace NAlice::NHollywood::NGeneralConversation
