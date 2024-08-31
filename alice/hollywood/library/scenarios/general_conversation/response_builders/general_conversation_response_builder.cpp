#include "general_conversation_response_builder.h"

#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/aggregated_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/easter_egg_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/error_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/generic_static_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/generative_tale_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/generative_toast_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/movie_akinator_strategy.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_sources/proactivity_strategy.h>

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/proactivity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/gif_card/gif_card.h>

#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/scenarios/features/gc.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>

#include <alice/library/analytics/scenario/builder.h>
#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/iterator/enumerate.h>

#include <util/random/shuffle.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

constexpr size_t MAX_ENTITY_HISTORY_SIZE = 15;

TString GetAnalyticIntent(const TStringBuf recognizedFrame, const TStringBuf intentName, const bool isPureGc) {
    if (recognizedFrame == FRAME_PURE_GC_ACTIVATE) {
        return "personal_assistant.scenarios.external_skill_gc";
    }
    if (recognizedFrame == FRAME_PURE_GC_DEACTIVATE) {
        return "personal_assistant.scenarios.pure_general_conversation_deactivation";
    }
    if (recognizedFrame == FRAME_GC_FEEDBACK) {
        return TStringBuilder() << "personal_assistant.feedback.gc_feedback_" << intentName;
    }

    if (intentName == INTENT_GC_PURE_GC_SESSION_TIMEOUT) {
        return "personal_assistant.scenarios.pure_general_conversation_deactivation.pure_gc_session_timeout";
    }
    if (intentName == INTENT_GC_PURE_GC_SESSION_DISABLED) {
        return "personal_assistant.scenarios.pure_general_conversation_deactivation.pure_gc_session_disabled";
    }

    if (isPureGc) {
        return "personal_assistant.scenarios.external_skill_gc";
    }
    if (recognizedFrame == FRAME_MICROINTENTS) {
        return TStringBuilder() << "personal_assistant.handcrafted." << intentName;
    }
    if (intentName == INTENT_DUMMY || GC_DUMMY_FRAMES.contains(recognizedFrame)) {
        return "personal_assistant.general_conversation.general_conversation_dummy";
    }
    if (intentName.StartsWith("generative_tale")) {
        return TStringBuilder() << "alice." << intentName;
    }

    return "personal_assistant.general_conversation.general_conversation";
}

TString GetAnalyticGcIntent(const TStringBuf recognizedFrame, const TStringBuf intentName, const TStringBuf originalIntent, const bool isPureGc) {
    if (recognizedFrame == FRAME_PURE_GC_ACTIVATE) {
        return "personal_assistant.scenarios.pure_general_conversation";
    }

    if (!isPureGc) {
        return "";
    }
    if (intentName == INTENT_DUMMY || GC_DUMMY_FRAMES.contains(recognizedFrame)) {
        return "personal_assistant.general_conversation.general_conversation_dummy";
    }
    if (recognizedFrame == FRAME_MICROINTENTS) {
        return ToString(originalIntent);
    }

    return "personal_assistant.general_conversation.general_conversation";
}

TString GetAnalyticOriginalIntent(const TStringBuf recognizedFrame, const TStringBuf intentName) {
    TStringBuilder builder;
    if (recognizedFrame == FRAME_LETS_DISCUSS_SPECIFIC_MOVIE) {
        if (intentName == INTENT_MOVIE_QUESTION) {
            return ToString(FRAME_LETS_DISCUSS_SPECIFIC_MOVIE);
        }
        return TStringBuilder() << "alice.general_conversation.general_conversation." << FRAME_LETS_DISCUSS_SPECIFIC_MOVIE;
    }
    if (recognizedFrame == FRAME_PROACTIVITY_ALICE_DO || recognizedFrame == FRAME_PROACTIVITY_BORED) {
        return ToString(recognizedFrame);
    }
    if (recognizedFrame == FRAME_MOVIE_DISCUSS || recognizedFrame == FRAME_GAME_DISCUSS || recognizedFrame == FRAME_MUSIC_DISCUSS) {
        return TStringBuilder() << "alice.general_conversation.general_conversation." << recognizedFrame;
    }
    if (intentName == INTENT_GC_PURE_GC_SESSION_TIMEOUT) {
        return TStringBuilder() << FRAME_PURE_GC_DEACTIVATE << "." << INTENT_GC_PURE_GC_SESSION_TIMEOUT;
    }
    //TODO: (deemonasd) remove
    if (intentName.StartsWith("alice.movie_discuss") || intentName.StartsWith("alice.game_discuss")) {
        return "alice.general_conversation.general_conversation";
    }
    if (intentName == INTENT_MOVIE_AKINATOR) {
        return ToString(recognizedFrame);
    }
    if (!recognizedFrame.empty() && !intentName.empty()) {
        return  TStringBuilder() << recognizedFrame << "." << intentName;
    }
    if (!recognizedFrame.empty() || !intentName.empty()) {
        return TStringBuilder() << recognizedFrame << intentName;
    }

    return "alice.general_conversation.general_conversation";
}

template <typename TContextWrapper>
void AddAction(TContextWrapper& contextWrapper, const TString& actionName, NScenarios::TFrameAction&& action, TGeneralConversationResponseWrapper* responseWrapper) {
    LOG_DEBUG(contextWrapper.Logger()) << action.GetParsedUtterance().GetTypedSemanticFrame().GetMusicPlaySemanticFrame().GetObjectId();
    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        LOG_DEBUG(contextWrapper.Logger()) << "Add run action";
        responseWrapper->Builder.GetResponseBodyBuilder()->AddAction(actionName, std::move(action));
    } else {
        LOG_DEBUG(contextWrapper.Logger()) << "Add continue action";
        responseWrapper->ContinueBuilder.GetResponseBodyBuilder()->AddAction(actionName, std::move(action));
    }
}

template <typename TContextWrapper>
void AddMovieOpenAction(TContextWrapper& contextWrapper, const TEntity& entity, TGeneralConversationResponseWrapper* responseWrapper) {
    if (!IsMovieOpenSupportedDevice(contextWrapper.RequestWrapper())) {
        return;
    }

    NScenarios::TFrameAction action;

    auto& parsedUtterance = *action.MutableParsedUtterance();

    const auto renderedUtterance = RenderMovieOpenUtterance(entity, contextWrapper);
    parsedUtterance.SetUtterance(renderedUtterance);

    auto& frame = *parsedUtterance.MutableFrame();
    frame.SetName(TString{NVideoCommon::SEARCH_VIDEO});

    auto& contentTypeSlot = *frame.AddSlots();
    contentTypeSlot.SetName(TString{NVideoCommon::SLOT_CONTENT_TYPE});
    contentTypeSlot.SetType(TString{NVideoCommon::SLOT_CONTENT_TYPE_TYPE});
    contentTypeSlot.SetValue(entity.GetMovie().GetType());

    auto& actionSlot = *frame.AddSlots();
    actionSlot.SetName(TString{NVideoCommon::SLOT_ACTION});
    actionSlot.SetType(TString{NVideoCommon::SLOT_ACTION_TYPE});
    actionSlot.SetValue(ToString(NVideoCommon::EVideoAction::Play));

    auto& searchTextSlot = *frame.AddSlots();
    searchTextSlot.SetName(TString{NVideoCommon::SLOT_SEARCH_TEXT});
    searchTextSlot.SetType("string");
    searchTextSlot.SetValue(entity.GetMovie().GetTitle());

    auto& nluHint = *action.MutableNluHint();
    nluHint.SetFrameName(TString{FRAME_MOVIE_OPEN});
    AddAction(contextWrapper, "movie_open_action", std::move(action), responseWrapper);
}

template <typename TContextWrapper>
void AddMusicOpenAction(TContextWrapper& contextWrapper, const TString& query, const TString& actionPurpose, TGeneralConversationResponseWrapper* responseWrapper) {
    NScenarios::TFrameAction action;
    auto& musicPlayFrame = *action.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame();
    musicPlayFrame.MutableSearchText()->SetStringValue(query);
    auto& analytics = *action.MutableParsedUtterance()->MutableAnalytics();
    analytics.SetPurpose(actionPurpose);
    auto& nluHint = *action.MutableNluHint();
    nluHint.SetFrameName("alice.proactivity.confirm");
    AddAction(contextWrapper, "music_open_action", std::move(action), responseWrapper);
}


template <typename TContextWrapper>
void AddMicrointentAction(TContextWrapper& contextWrapper, const TReplyInfo& replyInfo, TGeneralConversationResponseWrapper* responseWrapper) {
    const auto* microitnentInfo = contextWrapper.Resources().GetMicrointents().FindPtr(replyInfo.GetIntent());
    if (!microitnentInfo) {
        return;
    }
    const auto* musicAction = microitnentInfo->MusicActions.FindPtr(replyInfo.GetRenderedText());
    if (!musicAction) {
        return;
    }
    AddMusicOpenAction(contextWrapper, musicAction->Query, "microintent_music", responseWrapper);
}


void UpdateRecentDiscussedState(const TReplyInfo& replyInfo, TSessionState* sessionState) {
    const auto* entity = GetEntity(replyInfo, *sessionState);
    if (!entity) {
        return;
    }
    const auto entityKey = GetEntityKey(*entity);

    const THashSet<TString> discussed(sessionState->GetRecentDiscussedEntities().begin(), sessionState->GetRecentDiscussedEntities().end());
    if (discussed.contains(entityKey)) {
        return;
    }
    *sessionState->AddRecentDiscussedEntities() = entityKey;

    const size_t entityHistorySize = sessionState->GetRecentDiscussedEntities().size();
    if (entityHistorySize > MAX_ENTITY_HISTORY_SIZE) {
        const size_t oldEntityIndex = entityHistorySize - MAX_ENTITY_HISTORY_SIZE;
        auto& history = *sessionState->MutableRecentDiscussedEntities();
        history.erase(history.begin(), history.begin() + oldEntityIndex);
    }
}

void UpdateDiscussionState(const TReplyInfo& replyInfo, TSessionState* sessionState) {
    const auto& entityInfo = replyInfo.GetEntityInfo();
    if (IsEntitySet(entityInfo.GetEntity())) {
        auto& discussion = *sessionState->MutableEntityDiscussion();
        *discussion.MutableEntity() = entityInfo.GetEntity();
        discussion.SetDiscussionSentiment(entityInfo.GetDiscussionSentiment());
    }
}

template <typename TContextWrapper>
THolder<IReplySourceRenderStrategy> InitReplySourceRenderStrategy(TContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TMaybe<TReplyState>& replyState) {
        if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationApplyContextWrapper>) {
            switch (replyState->GetReplyInfo().GetReplySourceCase()) {
                case TReplyInfo::ReplySourceCase::kGenerativeTaleReply:
                    return MakeHolder<TGenerativeTaleRenderStrategy<TContextWrapper>>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                case TReplyInfo::ReplySourceCase::kAggregatedReply:
                case TReplyInfo::ReplySourceCase::kProactivityReply:
                case TReplyInfo::ReplySourceCase::kGenericStaticReply:
                case TReplyInfo::ReplySourceCase::kGenerativeToastReply:
                case TReplyInfo::ReplySourceCase::kMovieAkinatorReply:
                case TReplyInfo::ReplySourceCase::kEasterEggReply:
                case TReplyInfo::ReplySourceCase::kSeq2SeqReply:
                case TReplyInfo::ReplySourceCase::kNlgSearchReply:
                case TReplyInfo::ReplySourceCase::REPLYSOURCE_NOT_SET:
                    Y_ENSURE(false);
            }
        } else {
                if (!replyState) {
                    return MakeHolder<TErrorRenderStrategy>(contextWrapper, "timeout");
                }

                if (!replyState->HasReplyInfo()) {
                    //TODO(deemonasd): should by "all_filtered"
                    return MakeHolder<TErrorRenderStrategy>(contextWrapper, "dummy");
                }

                switch (replyState->GetReplyInfo().GetReplySourceCase()) {
                    case TReplyInfo::ReplySourceCase::kAggregatedReply:
                        return MakeHolder<TAggregatedRenderStrategy>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kProactivityReply:
                        return MakeHolder<TProactivityRenderStrategy>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kGenericStaticReply:
                        return MakeHolder<TGenericStaticRenderStrategy>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kGenerativeTaleReply:
                        return MakeHolder<TGenerativeTaleRenderStrategy<TContextWrapper>>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kGenerativeToastReply:
                        return MakeHolder<TGenerativeToastRenderStrategy>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kMovieAkinatorReply:
                        return MakeHolder<TMovieAkinatorRenderStrategy>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kEasterEggReply:
                        return MakeHolder<TEasterEggRenderStrategy>(contextWrapper, classificationResult, replyState->GetReplyInfo());
                    case TReplyInfo::ReplySourceCase::kSeq2SeqReply:
                    case TReplyInfo::ReplySourceCase::kNlgSearchReply:
                    case TReplyInfo::ReplySourceCase::REPLYSOURCE_NOT_SET:
                        Y_ENSURE(false);
                }
        }
}

void AddAnalyticsDeprecated(const TClassificationResult& classificationResult, const TMaybe<TReplyState>& replyState, TGeneralConversationResponseWrapper* responseWrapper) {
    // TODO (deemonasd): BOLTALKA-160
    TString intent;
    if (!replyState) {
        intent = "timeout";
    } else if (!replyState->HasReplyInfo()) {
        intent = "dummy";
    } else {
        intent = replyState->GetReplyInfo().GetIntent();
    }
    const auto& recognizedFrame = classificationResult.GetRecognizedFrame().GetName();
    const auto originalIntent = GetAnalyticOriginalIntent(recognizedFrame, intent);
    responseWrapper->GcResponseInfo.SetOriginalIntent(originalIntent);
    responseWrapper->GcResponseInfo.SetIntent(GetAnalyticIntent(recognizedFrame, intent, responseWrapper->SessionState.GetModalModeEnabled()));
    responseWrapper->GcResponseInfo.SetGcIntent(GetAnalyticGcIntent(recognizedFrame, intent, originalIntent, responseWrapper->SessionState.GetModalModeEnabled()));

    responseWrapper->GcResponseInfo.SetRecognizedFrame(recognizedFrame);
    responseWrapper->GcResponseInfo.SetIntentName(intent);
}

void AddAnalyticsFromSessionState(TGeneralConversationResponseWrapper* responseWrapper) {
    if (responseWrapper->SessionState.HasEntityDiscussion()) {
        const auto& entityDiscussion = responseWrapper->SessionState.GetEntityDiscussion();

        auto& discussionInfo = *responseWrapper->GcResponseInfo.MutableDiscussionInfo();
        discussionInfo.SetEntityKey(GetEntityKey(entityDiscussion.GetEntity()));
        discussionInfo.SetGivesNegativeFeedback(entityDiscussion.GetDiscussionSentiment() == TEntityDiscussion::NEGATIVE);
        responseWrapper->GcResponseInfo.MutableDiscussionInfo()->SetEntityKey(GetEntityKey(responseWrapper->SessionState.GetEntityDiscussion().GetEntity()));
    }

    responseWrapper->GcResponseInfo.SetEntitySearchCacheSize(responseWrapper->SessionState.GetEntitySearchCache().GetEntityKeys().size());
    responseWrapper->GcResponseInfo.SetIsPureGc(responseWrapper->SessionState.GetModalModeEnabled());
}

void AddFrameHint(const TStringBuf frameName, TResponseBodyBuilder* bodyBuilder) {
    TFrameNluHint nluHint;
    nluHint.SetFrameName(TString{frameName});
    bodyBuilder->AddNluHint(std::move(nluHint));
}

void AddGcFrames(TResponseBodyBuilder* bodyBuilder) {
    for (const auto& frameName : DEFAULT_GC_FRAMES) {
        AddFrameHint(frameName, bodyBuilder);
    }
}

} // namespace

template <typename TContextWrapper>
TGeneralConversationResponseBuilder<TContextWrapper>::TGeneralConversationResponseBuilder(TContextWrapper& contextWrapper,
        const TClassificationResult& classificationResult, const TSessionState& sessionState,
        const TMaybe<TReplyState>& replyState, const TMaybe<TVector<TNlgSearchReplyCandidate>>& suggestsState)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyState_(replyState)
    , SuggestsState_(suggestsState)
    , ReplySourceRenderStrategy_(InitReplySourceRenderStrategy(ContextWrapper_, ClassificationResult_, ReplyState_))
    , ResponseWrapper_(sessionState, ContextWrapper_.NlgWrapper())
    , sessionState_(sessionState)
{
}

template <typename TContextWrapper>
std::unique_ptr<NScenarios::TScenarioRunResponse> TGeneralConversationResponseBuilder<TContextWrapper>::BuildResponse(bool shouldContinue) && {
    if (ClassificationResult_.HasError()) {
        const auto& errorType = ClassificationResult_.GetError().GetType();
        const auto& errorMessage = ClassificationResult_.GetError().GetMessage();

        ResponseWrapper_.Builder.SetError(errorType, errorMessage);
        return std::move(ResponseWrapper_.Builder).BuildResponse();
    }

    if (ClassificationResult_.GetReplyInfo().GetIntent() == INTENT_GENERATIVE_TALE_FORCE_EXIT) {
        ResponseWrapper_.Builder.GetMutableFeatures().SetIsIrrelevant(true);
    }

    ResponseWrapper_.Builder.GetMutableFeatures().SetIgnoresExpectedRequest(ClassificationResult_.GetIgnoresExpectedRequest());

    if (shouldContinue) {
        ResponseWrapper_.Builder.SetFeaturesIntent(ClassificationResult_.GetRecognizedFrame().GetName());
        ResponseWrapper_.Builder.SetContinueArguments(ClassificationResult_);

        return std::move(ResponseWrapper_.Builder).BuildResponse();
    }

    TMaybe<TFrame> frame;
    if (ClassificationResult_.HasRecognizedFrame()) {
        frame = TFrame::FromProto(ClassificationResult_.GetRecognizedFrame());
    }
    const auto semanticFrame = ReplySourceRenderStrategy_->GetSemanticFrame();
    if (semanticFrame) {
        frame = TFrame::FromProto(*semanticFrame);
    }
    ResponseWrapper_.Builder.CreateResponseBodyBuilder(frame.Get());
    ResponseWrapper_.Builder.GetResponseBodyBuilder()->SetIsResponseConjugated(true);
    ReplySourceRenderStrategy_->AddResponse(&ResponseWrapper_);
    AddFeatures();
    AddResponseAfter();
    AddSuggestsBefore();
    ReplySourceRenderStrategy_->AddSuggests(&ResponseWrapper_);
    AddSuggestsAfter();
    AddShowViewDirective();
    FinalizeSessionState();
    FinalizeAnalytics();
    FinalizeBuilder();

    return std::move(ResponseWrapper_.Builder).BuildResponse();
}

template <typename TContextWrapper>
std::unique_ptr<NScenarios::TScenarioContinueResponse> TGeneralConversationResponseBuilder<TContextWrapper>::BuildContinueResponse() && {
    if (ClassificationResult_.HasError()) {
        const auto& errorType = ClassificationResult_.GetError().GetType();
        const auto& errorMessage = ClassificationResult_.GetError().GetMessage();

        ResponseWrapper_.ContinueBuilder.SetError(errorType, errorMessage);
        return std::move(ResponseWrapper_.ContinueBuilder).BuildResponse();
    }
    TMaybe<TFrame> frame;
    if (ClassificationResult_.HasRecognizedFrame()) {
        frame = TFrame::FromProto(ClassificationResult_.GetRecognizedFrame());
    }
    const auto semanticFrame = ReplySourceRenderStrategy_->GetSemanticFrame();
    if (semanticFrame) {
        frame = TFrame::FromProto(*semanticFrame);
    }
    ResponseWrapper_.ContinueBuilder.CreateResponseBodyBuilder(frame.Get());
    ResponseWrapper_.ContinueBuilder.GetResponseBodyBuilder()->SetIsResponseConjugated(true);
    ReplySourceRenderStrategy_->AddResponse(&ResponseWrapper_);
    ReplySourceRenderStrategy_->AddSuggests(&ResponseWrapper_);
    AddShowViewDirective();
    FinalizeSessionState();
    FinalizeAnalytics();
    FinalizeContinueBuilder();

    return std::move(ResponseWrapper_.ContinueBuilder).BuildResponse();
}

template <typename TContextWrapper>
TResponseBodyBuilder* TGeneralConversationResponseBuilder<TContextWrapper>::GetRunResponseBodyBuilder() {
    return ResponseWrapper_.Builder.GetResponseBodyBuilder();
}

template <typename TContextWrapper>
TResponseBodyBuilder* TGeneralConversationResponseBuilder<TContextWrapper>::GetContinueResponseBodyBuilder() {
    return ResponseWrapper_.ContinueBuilder.GetResponseBodyBuilder();
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::AddFeatures() {
    if (!ReplyState_) {
        return;
    }
    const auto& replyInfo = ReplyState_->GetReplyInfo();
    if (replyInfo.GetReplySourceCase() != TReplyInfo::ReplySourceCase::kAggregatedReply) {
        return;
    }
    const auto& aggregatedReply = replyInfo.GetAggregatedReply();

    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        auto* megamindGcFeatures = ResponseWrapper_.Builder.GetMutableFeatures().MutableGCFeatures();
        megamindGcFeatures->SetSeq2Seq(GetAggregatedReplySeq2Seq(aggregatedReply));
        megamindGcFeatures->SetDssmScore(GetAggregatedReplyDssmScore(aggregatedReply));
    }
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::AddResponseAfter() {
    if (!ReplyState_) {
        return;
    }

    if (const TEntity* entity = GetEntity(ReplyState_->GetReplyInfo(), ResponseWrapper_.SessionState); entity && !GetEntityKey(*entity).empty()) {
        AddMovieOpenAction(ContextWrapper_, *entity, &ResponseWrapper_);
    }
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_MICROINTENTS) {
        AddMicrointentAction(ContextWrapper_, ReplyState_->GetReplyInfo(), &ResponseWrapper_);
    }
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::AddSuggestsBefore() {
    const auto& requestWrapper = ContextWrapper_.RequestWrapper();
    auto& logger = ContextWrapper_.Logger();
    auto& nlgWrapper = ContextWrapper_.NlgWrapper();

    if (ResponseWrapper_.SessionState.GetModalModeEnabled()) {
        const auto& renderedPhrase = nlgWrapper.RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_pure_gc_deactivate_suggest", TNlgData{logger, requestWrapper}).Text;
        AddSuggest("suggest_pure_gc_deactivate", renderedPhrase, ToString(SUGGEST_TYPE),
                   /* forceGcResponse= */ false, *ResponseWrapper_.Builder.GetResponseBodyBuilder());
    }

    if (!ReplyState_) {
        return;
    }
    const auto& replyInfo = ReplyState_->GetReplyInfo();

    const bool isMovieDisscussionAllowedByDefault = IsMovieDisscussionAllowedByDefault(requestWrapper, ResponseWrapper_.SessionState.GetModalModeEnabled());
    const bool isQuestionAllowed = isMovieDisscussionAllowedByDefault || requestWrapper.HasExpFlag(EXP_HW_GC_ENTITY_DISCUSSION_QUESTION_SUGGEST);
    if (isQuestionAllowed && replyInfo.GetReplySourceCase() != TReplyInfo::ReplySourceCase::kMovieAkinatorReply) {
        if (const TMaybe<TString> question = GetQuestionAboutEntity(ContextWrapper_, ResponseWrapper_.SessionState, replyInfo)) {
            AddSuggest("suggest_question", *question, ToString(SUGGEST_TYPE),
                        /* forceGcResponse= */ true, *ResponseWrapper_.Builder.GetResponseBodyBuilder());
        }
    }

    const TEntity* entity = GetEntity(replyInfo, ResponseWrapper_.SessionState);
    if (entity && !GetEntityKey(*entity).empty() && IsMovieOpenSupportedDevice(requestWrapper)) {
        const auto suggestProb = GetExperimentTypedValue<double>(requestWrapper.ExpFlags(), EXP_HW_GC_MOVIE_OPEN_SUGGEST_PROB_PREFIX);
        if (isMovieDisscussionAllowedByDefault || (suggestProb && ContextWrapper_.Rng().RandomDouble() < *suggestProb)) {
            const auto moviePlaySuggest = RenderMovieOpenUtterance(*entity, ContextWrapper_);
            AddSuggest("suggest_play_movie", moviePlaySuggest, ToString(SUGGEST_TYPE),
                        /* forceGcResponse= */ false, *ResponseWrapper_.Builder.GetResponseBodyBuilder());
        }
    }
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::AddSuggestsAfter() {
    const auto& requestWrapper = ContextWrapper_.RequestWrapper();
    auto& logger = ContextWrapper_.Logger();
    auto& nlgWrapper = ContextWrapper_.NlgWrapper();

    if (SuggestsState_) {
        for (const auto& [suggestNumber, suggest] : Enumerate(SuggestsState_.GetRef())) {
            if (suggestNumber >= MAX_SUGGESTS_SIZE) {
                break;
            }
            TNlgData nlgData{logger, requestWrapper};
            nlgData.Context["text"] = suggest.GetText();
            const auto& renderedPhrase = nlgWrapper.RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_SUGGEST, nlgData).Text;
            AddSuggest("suggest_" + ToString(suggestNumber), renderedPhrase, ToString(SUGGEST_TYPE),
                       /* forceGcResponse= */ false, *ResponseWrapper_.Builder.GetResponseBodyBuilder());
        }
    }

    if (ClassificationResult_.GetUserLanguage() != ELang::L_RUS) {
        return;
    }

    if (ReplySourceRenderStrategy_->NeedCommonSuggests()) {
        bool isClientSupportSearch = !requestWrapper.ClientInfo().IsNavigator() &&
                                     !requestWrapper.ClientInfo().IsYaAuto() &&
                                     !requestWrapper.ClientInfo().IsSmartSpeaker() &&
                                     !requestWrapper.ClientInfo().IsElariWatch();
        bool isSearchRequired = !ResponseWrapper_.SessionState.GetModalModeEnabled();
        if (isClientSupportSearch && isSearchRequired && requestWrapper.Input().Utterance()) {
            AddSearchSuggest(requestWrapper.Input().Utterance(), *ResponseWrapper_.Builder.GetResponseBodyBuilder());
        }

        const auto& renderedPhrase = nlgWrapper.RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_what_can_you_do", TNlgData{logger, requestWrapper}).Text;
        AddSuggest("suggest_what_can_you_do", renderedPhrase, ToString(SUGGEST_TYPE),
                    /* forceGcResponse= */ false, *ResponseWrapper_.Builder.GetResponseBodyBuilder());
    }
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::AddShowViewDirective() {
    const auto& requestWrapper = ContextWrapper_.RequestWrapper();
    auto& logger = ContextWrapper_.Logger();

    if (!requestWrapper.HasExpFlag(EXP_HW_GC_RENDER_GENERAL_CONVERSATION) || !requestWrapper.Interfaces().GetSupportsShowView()) {
        return;
    }

    TResponseBodyBuilder* responseBodyBuilder;
    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        responseBodyBuilder = ResponseWrapper_.Builder.GetResponseBodyBuilder();
    } else {
        responseBodyBuilder = ResponseWrapper_.ContinueBuilder.GetResponseBodyBuilder();
    }
    AddShowView(logger, *responseBodyBuilder);
}


template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::FinalizeSessionState() {
    const auto& requestWrapper = ContextWrapper_.RequestWrapper();
    auto& sessionState = ResponseWrapper_.SessionState;
    sessionState.SetLastRequestServerTimeMs(ClassificationResult_.GetCurrentRequestServerTimeMs());
    sessionState.SetLastRequestSequenceNumber(ClassificationResult_.GetCurrentRequestSequenceNumber());

    if (ReplyState_ && ReplyState_->GetReplyInfo().GetReplySourceCase() != TReplyInfo::ReplySourceCase::kEasterEggReply && CountForEasterEggSuggest(requestWrapper)) {
        sessionState.MutableEasterEggState()->SetSuggestsClickCount(sessionState.GetEasterEggState().GetSuggestsClickCount() + 1);
    }

    if (ReplyState_ && ReplyState_->GetReplyInfo().GetEntityInfo().HasEntitySearchCache()) {
        *sessionState.MutableEntitySearchCache() = ReplyState_->GetReplyInfo().GetEntityInfo().GetEntitySearchCache();
    }

    if (ClassificationResult_.GetIsInvalidModalMode()) {
        sessionState.SetModalModeEnabled(false);
    }

    if (ReplyState_) {
        UpdateRecentDiscussedState(ReplyState_->GetReplyInfo(), &sessionState);
        UpdateDiscussionState(ReplyState_->GetReplyInfo(), &sessionState);
    }

    LOG_INFO(ContextWrapper_.Logger()) << "Session state in response: " << SerializeProtoText(sessionState);

    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        ResponseWrapper_.Builder.GetResponseBodyBuilder()->SetState(sessionState);
    } else {
        ResponseWrapper_.ContinueBuilder.GetResponseBodyBuilder()->SetState(sessionState);
    }
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::FinalizeAnalytics() {
    AddAnalyticsDeprecated(ClassificationResult_, ReplyState_, &ResponseWrapper_);
    AddAnalyticsFromSessionState(&ResponseWrapper_);
    ResponseWrapper_.GcResponseInfo.SetIsAggregatedRequest(ClassificationResult_.GetIsAggregatedRequest());
    ResponseWrapper_.GcResponseInfo.SetGcClassifierScore(ClassificationResult_.GetGcClassifierScore());

    TAnalyticsInfo::TObject object;
    *object.MutableGCResponseInfo() = ResponseWrapper_.GcResponseInfo;

    TResponseBodyBuilder* responseBodyBuilder;
    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        responseBodyBuilder = ResponseWrapper_.Builder.GetResponseBodyBuilder();
    } else {
        responseBodyBuilder = ResponseWrapper_.ContinueBuilder.GetResponseBodyBuilder();
    }

    auto& analyticsInfoBuilder = responseBodyBuilder->GetOrCreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.AddObject(object);
    analyticsInfoBuilder.SetIntentName(ResponseWrapper_.GcResponseInfo.GetIntent());
    analyticsInfoBuilder.AddSelectedSourceEvent(TInstant::MilliSeconds(ClassificationResult_.GetCurrentRequestServerTimeMs()), ResponseWrapper_.GcResponseInfo.GetOriginalIntent());

    if (ResponseWrapper_.SessionState.GetModalModeEnabled()) {
        analyticsInfoBuilder.SetProductScenarioName("external_skill_gc");
    } else if (ClassificationResult_.GetReplyInfo().GetIntent().StartsWith("generative_tale")) {
        analyticsInfoBuilder.SetProductScenarioName("generative_tale");
    } else {
        analyticsInfoBuilder.SetProductScenarioName("general_conversation");
    }
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::FinalizeBuilder() {
    if (ResponseWrapper_.SessionState.GetIsHeavyScenario() && ResponseWrapper_.SessionState.GetModalModeEnabled()) {
        AddGcFrames(ResponseWrapper_.Builder.GetResponseBodyBuilder());
    }
    if (ResponseWrapper_.SessionState.GetModalModeEnabled()) {
        if (ContextWrapper_.RequestWrapper().HasExpFlag(EXP_HW_GC_ENABLE_MODALITY_IN_PURE_GC)) {
            ResponseWrapper_.Builder.GetResponseBodyBuilder()->SetExpectsRequest(true);
        } else {
            AddFrameHint(FRAME_PURE_GC_DEACTIVATE, ResponseWrapper_.Builder.GetResponseBodyBuilder());
        }
    }

    if (ClassificationResult_.GetIsFrameFeatured()) {
        ResponseWrapper_.Builder.SetFeaturesIntent(ClassificationResult_.GetRecognizedFrame().GetName());
    } else {
        ResponseWrapper_.Builder.SetFeaturesIntent(ResponseWrapper_.GcResponseInfo.GetIntent());
    }

    ResponseWrapper_.Builder.GetResponseBodyBuilder()->SetShouldListen(ReplySourceRenderStrategy_->ShouldListen());
}

template <typename TContextWrapper>
void TGeneralConversationResponseBuilder<TContextWrapper>::FinalizeContinueBuilder() {
    ResponseWrapper_.ContinueBuilder.GetResponseBodyBuilder()->SetShouldListen(ReplySourceRenderStrategy_->ShouldListen());
}

template class TGeneralConversationResponseBuilder<TGeneralConversationRunContextWrapper>;
template class TGeneralConversationResponseBuilder<TGeneralConversationApplyContextWrapper>;

}  // namespace NAlice::NHollywood::NGeneralConversation
