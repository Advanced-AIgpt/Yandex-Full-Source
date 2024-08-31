#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <util/generic/string.h>

namespace NAlice::NHollywood::NGeneralConversation {

TString GetEntityKey(const TEntity& entity);
TString GetMovieEntityKey(ui32 kpId);
TString GetMusicBandEntityKey(const TString& id);
TString GetVideoGameEntityKey(const TString& id);

bool IsEntitySet(const TEntity& entity);
bool RequiresSentimentForDiscussion(const TEntity& entity);

const TEntitySearchCache* GetEntitySearchCache(const TReplyInfo& replyInfo, const TSessionState& sessionState);
const TEntity* GetEntity(const TReplyInfo& replyInfo, const TSessionState& sessionState);
TEntityDiscussion::EDiscussionSentiment GetDiscussionSentiment(const TReplyInfo& replyInfo,
                                                               const TSessionState& sessionState);

} // namespace NAlice::NHollywood::NGeneralConversation
