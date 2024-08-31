#include "entity.h"

#include <util/string/cast.h>

namespace NAlice::NHollywood::NGeneralConversation {

TString GetEntityKey(const TEntity& entity) {
    switch (entity.GetEntityCase()) {
        case TEntity::EntityCase::kMovie:
            return GetMovieEntityKey(entity.GetMovie().GetId());
        case TEntity::EntityCase::kMusicBand:
            return GetMusicBandEntityKey(entity.GetMusicBand().GetId());
        case TEntity::EntityCase::kVideoGame:
            return GetVideoGameEntityKey(entity.GetVideoGame().GetId());
        case TEntity::EntityCase::ENTITY_NOT_SET:
            return "";
    }
}

TString GetMovieEntityKey(ui32 kpId) {
    if (kpId == 0) {
        return "";
    }

    return "movie:" + ToString(kpId);
}

TString GetMusicBandEntityKey(const TString& id) {
    return "music_band:" + id;
}

TString GetVideoGameEntityKey(const TString& id) {
    return "video_game:" + id;
}

bool IsEntitySet(const TEntity& entity) {
    return entity.GetEntityCase() != TEntity::EntityCase::ENTITY_NOT_SET;
}

bool RequiresSentimentForDiscussion(const TEntity& entity) {
    return entity.GetEntityCase() == TEntity::EntityCase::kMovie;
}

const TEntity* GetEntity(const TReplyInfo& replyInfo, const TSessionState& sessionState) {
    if (IsEntitySet(replyInfo.GetEntityInfo().GetEntity())) {
        return &replyInfo.GetEntityInfo().GetEntity();
    }

    if (sessionState.HasEntityDiscussion()) {
        const auto& entity = sessionState.GetEntityDiscussion().GetEntity();
        if (IsEntitySet(entity)) {
            return &entity;
        }
    }

    return nullptr;
}

const TEntitySearchCache* GetEntitySearchCache(const TReplyInfo& replyInfo, const TSessionState& sessionState) {
    if (replyInfo.GetEntityInfo().HasEntitySearchCache()) {
        return &replyInfo.GetEntityInfo().GetEntitySearchCache();
    }
    if (sessionState.HasEntitySearchCache()) {
        return &sessionState.GetEntitySearchCache();
    }

    return nullptr;
}

TEntityDiscussion::EDiscussionSentiment GetDiscussionSentiment(const TReplyInfo& replyInfo,
                                                               const TSessionState& sessionState)
{
    if (replyInfo.GetEntityInfo().GetDiscussionSentiment() != TEntityDiscussion::UNDEFINED) {
        return replyInfo.GetEntityInfo().GetDiscussionSentiment();
    }

    if (sessionState.HasEntityDiscussion()) {
        return sessionState.GetEntityDiscussion().GetDiscussionSentiment();
    }

    return TEntityDiscussion::UNDEFINED;
}

} // namespace NAlice::NHollywood::NGeneralConversation
