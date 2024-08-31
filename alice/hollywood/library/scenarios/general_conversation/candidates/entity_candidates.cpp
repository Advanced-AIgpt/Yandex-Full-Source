#include "entity_candidates.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <util/string/cast.h>
#include <util/string/split.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

constexpr TStringBuf ENTITY_CANDIDATES_REQUEST_ITEM = "hw_entity_search_request";
constexpr TStringBuf ENTITY_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM = "hw_entity_search_request_rtlog_token";
constexpr TStringBuf ENTITY_CANDIDATES_SEARCH_RESPONSE_ITEM = "hw_entity_search_response";

constexpr TStringBuf DEFAULT_CANDIDATES_PATH = "/get?obj=lst.rec&export=json&rearr=entity_recommender_list_experiment=";
constexpr TStringBuf DEFAULT_EXPERIMENT = "last_viewed_films";
constexpr TStringBuf CANDIDATES_HANDLE_AUTHORIZATION = "&client=general_conversation&passportid=";

TString ConstructEntityCandidatesUrl(const TScenarioRunRequestWrapper& requestWrapper) {
    const auto uid = GetUid(requestWrapper);
    const auto experiment = GetExperimentTypedValue<TString>(requestWrapper.ExpFlags(), EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH_EXP).GetOrElse(ToString(DEFAULT_EXPERIMENT));

    return TString::Join(DEFAULT_CANDIDATES_PATH, experiment, CANDIDATES_HANDLE_AUTHORIZATION, uid);
}

TMaybe<TEntitySearchCache> ExtractEntityCandidates(const TScenarioRunRequestWrapper& requestWrapper, const NJson::TJsonValue& docs) {
    if (docs["cards"][0]["parent_collection"]["id"].GetString() == "lstfp") {
        return Nothing();
    }

    TEntitySearchCache result;
    result.SetLastUpdateTimeMs(GetServerTimeMs(requestWrapper));
    const auto& moviesRaw = docs["cards"][0]["parent_collection"]["object"].GetArray();
    for (const auto& movieDesc : moviesRaw) {
        TString prefix;
        ui32 kpId;
        if (StringSplitter(movieDesc["ids"]["kinopoisk"].GetString()).Split('/').TryCollectInto(&prefix, &kpId)) {
            *result.AddEntityKeys() = GetMovieEntityKey(kpId);
        }
    }

    return result;
}

void FilterUnknown(const TGeneralConversationResources& resources, TVector<TString>* entities) {
    EraseIf(*entities, [&resources] (const auto& id) { return !resources.GetEntity(id); });
}

void FilterDiscussed(const TSessionState& sessionState, TVector<TString>* entities) {
    const THashSet<TString> discussed(sessionState.GetRecentDiscussedEntities().begin(), sessionState.GetRecentDiscussedEntities().end());
    EraseIf(*entities, [&discussed] (const auto& entity) { return discussed.contains(entity); });
}

void FilterByMovieType(const TReplyInfo& replyInfo, const TGeneralConversationResources& resources, TVector<TString>* entities) {
    if (!replyInfo.GetEntityInfo().HasEntity()) {
        return;
    }
    const auto movieType = replyInfo.GetEntityInfo().GetEntity().GetMovie().GetType();
    if (movieType.empty()) {
        return;
    }

    EraseIf(*entities, [&resources, &movieType] (const auto& id) { return resources.GetEntity(id)->GetMovie().GetType() != movieType; });
}

const TString ChooseEntityKey(const TVector<TString>& ids, TGeneralConversationRunContextWrapper& contextWrapper) {
    auto id = ids.front();
    if (!contextWrapper.RequestWrapper().HasExpFlag(EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH_FIRST)) {
        id = ids[contextWrapper.Rng().RandomInteger() % ids.size()];
    }

    return id;
}

} // namespace

void AddEntityCandidatesRequest(const TScenarioRunRequestWrapper& requestWrapper, TScenarioHandleContext* ctx) {
    const auto url = ConstructEntityCandidatesUrl(requestWrapper);
    const auto request = PrepareHttpRequest(url, ctx->RequestMeta, ctx->Ctx.Logger(), ToString(ENTITY_CANDIDATES_REQUEST_ITEM));
    AddHttpRequestItems(*ctx, request, ENTITY_CANDIDATES_REQUEST_ITEM, ENTITY_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM);
}

TMaybe<TEntitySearchCache> RetireEntityCandidatesResponse(const TScenarioRunRequestWrapper& requestWrapper, const TScenarioHandleContext& ctx) {
    const auto jsonResponseMaybe = RetireHttpResponseJsonMaybe(ctx, ENTITY_CANDIDATES_SEARCH_RESPONSE_ITEM, ENTITY_CANDIDATES_REQUEST_RTLOG_TOKEN_ITEM, /* logBody = */ false);
    if (jsonResponseMaybe) {
        return ExtractEntityCandidates(requestWrapper, jsonResponseMaybe.GetRef());
    }

    return Nothing();
}

void FillEntity(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState, TReplyInfo* replyInfo) {
    const TEntitySearchCache* entitySearchCachePtr = GetEntitySearchCache(*replyInfo, sessionState);

    if (entitySearchCachePtr) {
        TVector<TString> entities(entitySearchCachePtr->GetEntityKeys().begin(), entitySearchCachePtr->GetEntityKeys().end());
        FilterUnknown(contextWrapper.Resources(), &entities);
        FilterDiscussed(sessionState, &entities);
        FilterByMovieType(*replyInfo, contextWrapper.Resources(), &entities);
        const auto cropSize = GetExperimentTypedValue<ui64>(contextWrapper.RequestWrapper().ExpFlags(), EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH_CROP).GetOrElse(entities.size());
        entities.crop(cropSize);
        if (!entities.empty() && TrySetEntity(ChooseEntityKey(entities, contextWrapper), contextWrapper.RequestWrapper(), contextWrapper.Resources(), contextWrapper.Rng(), replyInfo)) {
            return;
        }
    }
    if (replyInfo->GetEntityInfo().HasEntity()) {
        const auto movieType = replyInfo->GetEntityInfo().GetEntity().GetMovie().GetType();
        if (!movieType.empty()) {
            const auto& moviesToDiscuss = contextWrapper.Resources().GetMoviesToDiscussByType(movieType);
            const auto kpId = moviesToDiscuss[contextWrapper.Rng().RandomInteger() % moviesToDiscuss.size()].GetId();
            if (TrySetEntity(GetMovieEntityKey(kpId), contextWrapper.RequestWrapper(), contextWrapper.Resources(), contextWrapper.Rng(), replyInfo)) {
                return;
            }
        }
    }

    const auto& moviesToDiscuss = contextWrapper.Resources().GetMoviesToDiscuss();
    const auto kpId = moviesToDiscuss[contextWrapper.Rng().RandomInteger() % moviesToDiscuss.size()].GetId();
    TrySetEntity(GetMovieEntityKey(kpId), contextWrapper.RequestWrapper(), contextWrapper.Resources(), contextWrapper.Rng(), replyInfo);
}

void FillEntityType(TGeneralConversationRunContextWrapper& contextWrapper, bool contentForChild, TReplyInfo* replyInfo) {
    auto& rng = contextWrapper.Rng();

    const auto& contentTypesRef = contentForChild ? CHILD_CONTENT_TYPES : KNOWN_MOVIE_CONTENT_TYPES;
    TVector<TStringBuf> contentTypes(contentTypesRef.begin(), contentTypesRef.end());
    Sort(contentTypes.begin(), contentTypes.end());
    replyInfo->MutableEntityInfo()->MutableEntity()->MutableMovie()->SetType(ToString(contentTypes[rng.RandomInteger(contentTypes.size())]));
}

bool TrySetEntity(const TString& entityKey, const TScenarioRunRequestWrapper& requestWrapper, const TGeneralConversationResources& resources, IRng& rng, TReplyInfo* replyInfo) {
    const auto* entity = resources.GetEntity(entityKey);
    if (!entity) {
        return false;
    }
    auto& entityInfo = *replyInfo->MutableEntityInfo();
    *entityInfo.MutableEntity() = *entity;

    if (!RequiresSentimentForDiscussion(*entity)) {
        return true;
    }

    const auto negativeAnswerFraction = entity->GetMovie().GetNegativeAnswerFraction();
    if (negativeAnswerFraction == 0.) {
        entityInfo.SetDiscussionSentiment(TEntityDiscussion::POSITIVE);
    } else if (negativeAnswerFraction == 1.) {
        entityInfo.SetDiscussionSentiment(TEntityDiscussion::NEGATIVE);
    } else {
        const auto forcedSentiment = GetExperimentValueWithPrefix(requestWrapper.ExpFlags(), EXP_HW_GC_FORCE_ENTITY_DISCUSSION_SENTIMENT_PREFIX);
        if (forcedSentiment == "positive") {
            entityInfo.SetDiscussionSentiment(TEntityDiscussion::POSITIVE);
        } else if (forcedSentiment == "negative") {
            entityInfo.SetDiscussionSentiment(TEntityDiscussion::NEGATIVE);
        }
    }

    if (entityInfo.GetDiscussionSentiment() == TEntityDiscussion::UNDEFINED) {
        if (rng.RandomDouble() < negativeAnswerFraction) {
            entityInfo.SetDiscussionSentiment(TEntityDiscussion::NEGATIVE);
        } else {
            entityInfo.SetDiscussionSentiment(TEntityDiscussion::POSITIVE);
        }
    }

    return true;
}

template <typename TRequestWrapper>
bool IsMovieOpenSupportedDevice(const TRequestWrapper& requestWrapper) {
    return requestWrapper.ClientInfo().IsSmartSpeaker()
        && requestWrapper.Proto().GetBaseRequest().GetInterfaces().GetIsTvPlugged();
}

template bool IsMovieOpenSupportedDevice(const TScenarioRunRequestWrapper&);
template bool IsMovieOpenSupportedDevice(const TScenarioApplyRequestWrapper&);


template <typename TContextWrapper>
TString RenderMovieOpenUtterance(const TEntity& entity, TContextWrapper& contextWrapper) {
    Y_ENSURE(IsMovieOpenSupportedDevice(contextWrapper.RequestWrapper()));

    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["movie_title"] = entity.GetMovie().GetTitle();
    nlgData.Context["movie_type"] = entity.GetMovie().GetType();

    return contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_movie_open_utterance", nlgData).Text;
}

template TString RenderMovieOpenUtterance(const TEntity& , TGeneralConversationRunContextWrapper&);
template TString RenderMovieOpenUtterance(const TEntity& , TGeneralConversationApplyContextWrapper&);


template <typename TContextWrapper>
TMaybe<TString> GetQuestionAboutEntity(TContextWrapper& contextWrapper, const TSessionState& sessionState,
        const TReplyInfo& replyInfo)
{
    const TEntity* entity = GetEntity(replyInfo, sessionState);
    if (!entity) {
        return Nothing();
    }
    auto discussionSentiment = GetDiscussionSentiment(replyInfo, sessionState);
    if (discussionSentiment == TEntityDiscussion::UNDEFINED) {
        LOG_WARN(contextWrapper.Logger()) << "Discussion sentiment was undefined, set to positive";
        discussionSentiment = TEntityDiscussion::POSITIVE;
    }

    const auto entityKey = GetEntityKey(*entity);

    if (const auto* questionsPtr = contextWrapper.Resources().GetEntityQuestions(entityKey, discussionSentiment)) {
        if (!questionsPtr->empty()) {
            const auto questionIndex = contextWrapper.Rng().RandomInteger(questionsPtr->size());
            return (*questionsPtr)[questionIndex];
        }
    }

    return Nothing();
}

template TMaybe<TString> GetQuestionAboutEntity(TGeneralConversationRunContextWrapper&, const TSessionState&, const TReplyInfo&);
template TMaybe<TString> GetQuestionAboutEntity(TGeneralConversationApplyContextWrapper&, const TSessionState&, const TReplyInfo&);


void UpdateLastDiscussion(const TClassificationResult& classificationResult, TSessionState* sessionState) {
    auto& discussion = *sessionState->MutableEntityDiscussion();
    discussion.SetLastTimestampMs(classificationResult.GetCurrentRequestServerTimeMs());
    discussion.SetLastSequenceNumber(classificationResult.GetCurrentRequestSequenceNumber());
}

} // namespace NAlice::NHollywood::NGeneralConversation
