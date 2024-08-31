#include "run_candidates_handle.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/aggregated_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/filter_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/filter_by_embedding_model.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/generative_tale_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/proactivity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/seq2seq_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_fast_data.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/gif_card/gif_card.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/string_utils/subst_buf/substbuf.h>

#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

constexpr float RANDOM_GIF_PROBABILITY = 0.3;
constexpr float REPEAT_FILTER_BORDER = 0.9;
constexpr size_t MAX_SLEEP_USEC = 200000;
const THashSet<TString> ALLOWED_SCENARIOS_CROSSPROMO = {"GeneralConversation", "Search"};
const THashSet<wchar_t> END_OF_SENTENCE_CHARS = {'.', '!', '?'};


void EmbedCandidates(const TScenarioRunRequestWrapper& requestWrapper, const TGeneralConversationResources& resources, TVector<TAggregatedReplyCandidate>* candidates) {
    THashSet<TString> appliedModels;
    const auto rankerName = GetExperimentValueWithPrefix(requestWrapper.ExpFlags(), EXP_HW_GC_RANKER_NAME_PREFIX);
    if (rankerName) {
        const auto* model = resources.GetRanker(rankerName.GetRef());
        if (model) {
            appliedModels.emplace(model->GetEmbedderName());
        }
    }
    for (const auto& modelName : appliedModels) {
        const auto* embedder = resources.GetEmbedder(modelName);
        if (!embedder) {
            continue;
        }
        for (auto& candidate : *candidates) {
            const auto embedding = embedder->Embed(GetAggregatedReplyText(candidate));
            TEmbedding protoEmbedding;
            protoEmbedding.MutableValue()->Reserve(embedding.size());
            for (const auto& number : embedding) {
                protoEmbedding.AddValue(number);
            }
            (*candidate.MutableEmbeddings())[modelName] = std::move(protoEmbedding);
        }
    }
}

void RankCandidates(const TScenarioRunRequestWrapper& requestWrapper, const TGeneralConversationResources& resources,
        const TSessionState& sessionState, TVector<TAggregatedReplyCandidate>* candidates) {
    const auto maybeModelName = GetExperimentValueWithPrefix(requestWrapper.ExpFlags(), EXP_HW_GC_RANKER_NAME_PREFIX);
    if (!maybeModelName) {
        return;
    }
    const auto rankerWeight = GetExperimentTypedValue<float>(requestWrapper.ExpFlags(), EXP_HW_GC_RANKER_WEIGHT_PREFIX);
    if (!rankerWeight) {
        return;
    }

    const auto* memoryStates = requestWrapper.GetDataSource(EDataSourceType::GC_MEMORY_STATE);
    if (!memoryStates) {
        return;
    }

    const TString modelName(maybeModelName.GetRef());
    const auto state = memoryStates->GetBegemotGcMemoryState().GetModelState().find(modelName);
    if (state == memoryStates->GetBegemotGcMemoryState().GetModelState().end()) {
        return;
    }
    const auto* model = resources.GetRanker(modelName);
    if (!model) {
        return;
    }

    model->Rerank(*candidates, state->second, rankerWeight.GetRef(), sessionState.GetModalModeEnabled());
}

void FilterProactivity(TVector<TAggregatedReplyCandidate>* candidates) {
    EraseIf(*candidates, [](const auto& candidate) { return GetAggregatedReplySource(candidate) == SOURCE_PROACTIVITY; });
}

TMaybe<TString> GetFactsCrossPromo(const TString& entity, TGeneralConversationRunContextWrapper& contextWrapper,
                                   const TSessionState& sessionState)
{
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const auto& resources = contextWrapper.Resources();
    const auto fastData = contextWrapper.FastData();
    THashSet<ui64> historyFactsHash;
    {
        const auto& history = sessionState.GetFactsCrosspromoHashHistory();
        historyFactsHash.reserve(history.size());
        for (const auto& factHash: history) {
            historyFactsHash.insert(factHash);
        }
    }

    const auto& crosspromoFacts = resources.GetCrossPromoFacts(!requestWrapper.HasExpFlag(EXP_HW_FACTS_CROSSPROMO_FULL_DICT));
    if (const auto* listItemsPtr = crosspromoFacts.FindPtr(entity)) {
        TVector<TString> filteredFacts;
        fastData->CopyIfNotInBanlistFactsCrosspromo(*listItemsPtr, &filteredFacts);
        EraseIf(filteredFacts, [&] (const auto& fact) { return historyFactsHash.contains(THash<TString>{}(fact)); });
        if (!filteredFacts.empty()) {
            return filteredFacts[contextWrapper.Rng().RandomInteger() % filteredFacts.size()];
        }
    }
    return Nothing();
}

TMaybe<TString> GetFactsCrossPromoEntity(const TScenarioRunRequestWrapper& requestWrapper) {
    const auto frame = requestWrapper.Input().FindSemanticFrame(FRAME_FACTS_CROSSPROMO);
    if (!frame) {
        return Nothing();
    }
    for (const auto& slot : frame->GetSlots()) {
        if (slot.GetName() == SLOT_FACTS_CROSSPROMO) {
            return slot.GetValue();
        }
    }
    return Nothing();
}

bool AddPeriod(const TString& text) {
    if (!text.empty() && !END_OF_SENTENCE_CHARS.contains(text[text.size() - 1])){
        return true;
    }
    return false;
}

bool IsQuestion(const TString& text) {
    if (!text.empty() && text[text.size() - 1] == '?') {
        return true;
    }
    return false;
}

bool AllowedLastScenario(const TScenarioRunRequestWrapper& requestWrapper) {
    const auto* dialogHistoryDataSource = requestWrapper.GetDataSource(EDataSourceType::DIALOG_HISTORY);
    Y_ENSURE(dialogHistoryDataSource, "DialogHistoryDataSource not found in the run request");
    const auto& dialogTurns = dialogHistoryDataSource->GetDialogHistory().GetDialogTurns();
    if (dialogTurns.empty()) {
        return false;
    }
    const auto& dialogLastTurn = dialogTurns[dialogTurns.size() - 1];
    const auto forbiddenPrevScenarioTimeoutMs = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_FACTS_CROSSPROMO_FORBIDDEN_PREV_SCENARIO_TIMEOUT).GetOrElse(CROSSPROMO_FORBIDDEN_PREV_SCENARIO_TIMEOUT);
    if (dialogLastTurn.GetServerTimeMs() + forbiddenPrevScenarioTimeoutMs <= GetServerTimeMs(requestWrapper)) {
        return false;
    }
    return ALLOWED_SCENARIOS_CROSSPROMO.contains(dialogLastTurn.GetScenarioName());
}

void AddPromoFact(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState, TReplyInfo* replyInfo) {
    const auto& requestWrapper = contextWrapper.RequestWrapper();

    const auto entity = GetFactsCrossPromoEntity(requestWrapper);
    const auto crosspromoTimeoutMs = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_FACTS_CROSSPROMO_TIMEOUT).GetOrElse(FACTS_CROSSPROMO_TIMEOUT_MS);
    const auto currentServerTimeMs = GetServerTimeMs(requestWrapper);
    const auto crosspromoRequestServerTimeMs = sessionState.GetLastFactsCrosspromoRequestServerTimeMs();
    if (!entity.Defined() || crosspromoRequestServerTimeMs + crosspromoTimeoutMs >= currentServerTimeMs) {
        return;
    }
    const auto& age = requestWrapper.Proto().GetBaseRequest().GetUserClassification().GetAge();
    if (age == 0 && requestWrapper.HasExpFlag(EXP_HW_FACTS_CROSSPROMO_ONLY_CHILDREN)) {
        return;
    }
    const auto& text = GetAggregatedReplyText(replyInfo->GetAggregatedReply());
    if (IsQuestion(text) && !requestWrapper.HasExpFlag(EXP_HW_FACTS_CROSSPROMO_CHANGE_QUESTIONS)) {
        return;
    }
    if (!AllowedLastScenario(requestWrapper) && !requestWrapper.HasExpFlag(EXP_HW_FACTS_CROSSPROMO_SCENARIO_FILTER_DISABLE)) {
        return;
    }

    const auto crosspromoResult = GetFactsCrossPromo(entity.GetRef(), contextWrapper, sessionState);
    if (crosspromoResult.Defined()) {
        auto& factsCrossPromoInfo = *replyInfo->MutableFactsCrossPromoInfo();
        factsCrossPromoInfo.SetFactsCrossPromoText(crosspromoResult.GetRef());
        factsCrossPromoInfo.SetFactsCrossPromoEntityKey(entity.GetRef());
        factsCrossPromoInfo.SetFactsCrossPromoPeriod(AddPeriod(text));
    }
}

void AddGifCard(TGeneralConversationRunContextWrapper& contextWrapper, TReplyInfo* replyInfo) {
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const auto& resources = contextWrapper.Resources();
    const auto fastData = contextWrapper.FastData();
    auto& rng = contextWrapper.Rng();
    const auto& gifsAndEmojiInfo = replyInfo->GetGifsAndEmojiInfo();

    if (!replyInfo->HasAggregatedReply()) {
        return;
    }
    if (!requestWrapper.HasExpFlag(EXP_HW_GC_ENABLE_GIF_SHOW)) {
        return;
    }
    if (!requestWrapper.BaseRequestProto().GetInterfaces().GetCanShowGif()) {
        return;
    }
    if (requestWrapper.ContentRestrictionLevel() == EContentSettings::children ||
        requestWrapper.ContentRestrictionLevel() == EContentSettings::safe){
        return;
    }

    TVector<const TGif*> gifs;
    if (replyInfo->GetAggregatedReply().HasNlgSearchReply()) {
        for (const auto& gif : replyInfo->GetAggregatedReply().GetNlgSearchReply().GetGifs()) {
            gifs.push_back(&gif);
        }
    }

    if (requestWrapper.HasExpFlag(EXP_HW_GC_ADD_RANDOM_GIF_TO_ANSWER)) {
        const auto gifProbability = GetExperimentTypedValue<float>(requestWrapper.ExpFlags(), EXP_HW_GC_RANDOM_GIF_PROBABILITY).GetOrElse(RANDOM_GIF_PROBABILITY);
        if (rng.RandomDouble() < gifProbability) {
            const auto& unclassifiedGifs = resources.GetUnclassifiedGifs();
            if (unclassifiedGifs.empty()) {
                return;
            }
            gifs.push_back(&unclassifiedGifs[rng.RandomInteger() % gifs.size()]);
        }
    } else if (requestWrapper.HasExpFlag(EXP_HW_GC_ADD_GIF_TO_ANSWER) && gifsAndEmojiInfo.GetEmoji()) {
        if (const auto* emotionalGifs = resources.GetEmotionalGifs().FindPtr(gifsAndEmojiInfo.GetEmoji())) {
            gifs.push_back(&(*emotionalGifs)[rng.RandomInteger() % emotionalGifs->size()]);
        }
    }
    fastData->FilterGifByBanlist(&gifs);
    if (gifs.empty()) {
        return;
    }

    *replyInfo->MutableGifsAndEmojiInfo()->MutableGif() = *gifs[rng.RandomInteger() % gifs.size()];
}

TAggregatedReplyCandidate ChooseReply(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sesstionState, const TVector<TAggregatedReplyCandidate>& candidates) {
    Y_ENSURE(!candidates.empty());
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    auto& rng = contextWrapper.Rng();
    const auto isMovieDisscussionAllowedByDefault = IsMovieDisscussionAllowedByDefault(requestWrapper, sesstionState.GetModalModeEnabled());

    if (requestWrapper.HasExpFlag(EXP_HW_FORCE_GC_ENTITY_SOFT)) {
        for (const auto& candidate : candidates) {
            if (FindPtr(SOURCES_ENTITY, GetAggregatedReplySource(candidate))) {
                return candidate;
            }
        }
    }

    if (isMovieDisscussionAllowedByDefault || requestWrapper.HasExpFlag(EXP_HW_FORCE_GC_ENTITY_SOFT_RNG)) {
        TVector<const TAggregatedReplyCandidate*> candidatesCopy;
        candidatesCopy.reserve(candidates.size());
        for (const auto& candidate : candidates) {
            candidatesCopy.push_back(&candidate);
        }
        EraseIf(candidatesCopy, [](const auto* candidate) { return !FindPtr(SOURCES_ENTITY, GetAggregatedReplySource(*candidate)); });
        if (!candidatesCopy.empty()) {
            return *candidatesCopy[rng.RandomInteger(candidatesCopy.size())];
        }
    }

    if (isMovieDisscussionAllowedByDefault || requestWrapper.HasExpFlag(EXP_HW_FORCE_GC_PROACTIVITY_SOFT)) {
        for (const auto& candidate : candidates) {
            if (GetAggregatedReplySource(candidate) == SOURCE_PROACTIVITY) {
                return candidate;
            }
        }
    }

    if (requestWrapper.HasExpFlag(EXP_HW_FORCE_GC_SEARCH_GIF_SOFT)) {
        for (const auto& candidate : candidates) {
            if (!candidate.GetNlgSearchReply().GetGifs().empty()) {
                return candidate;
            }
        }
    }

    if (requestWrapper.HasExpFlag(EXP_HW_GC_FIRST_REPLY)) {
        return candidates[0];
    }

    if (requestWrapper.HasExpFlag(EXP_HW_GC_SAMPLE_POLICY)) {
        double relevSum = 0;
        for (const auto& candidate : candidates) {
            relevSum += candidate.GetRelevance();
        }
        auto threshold = rng.RandomDouble(0, relevSum);
        double cumSum = 0;
        for (const auto& candidate : candidates) {
            cumSum += candidate.GetRelevance();
            if (cumSum >= threshold) {
                return candidate;
            }
        }
    }

    return candidates[rng.RandomInteger() % candidates.size()];
}

TVector<float> GetReplyEmbedding(const TAggregatedReplyCandidate& candidate, const NAlice::TBoltalkaDssmEmbedder* embedder, TString modelName) {
    const auto precomputedEmbedding = candidate.GetEmbeddings().find(modelName);
    if (precomputedEmbedding != candidate.GetEmbeddings().end()) {
        return {precomputedEmbedding->second.GetValue().begin(), precomputedEmbedding->second.GetValue().end()};
    }
    return embedder->Embed(GetAggregatedReplyText(candidate));
}

TMaybe<TAggregatedReplyCandidate> GetAggregatedReply(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TSessionState& sessionState) {
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const auto& resources = contextWrapper.Resources();
    const auto fastData = contextWrapper.FastData();
    const bool isProactivityAllowed = classificationResult.GetIsProactivityTimeAllowed() && classificationResult.GetIsProactivityDialogHistoryAllowed();

    auto candidates = RetireAggregatedReplyCandidatesResponse(contextWrapper, classificationResult);

    EmbedCandidates(requestWrapper, resources, &candidates);
    RankCandidates(requestWrapper, resources, sessionState, &candidates);
    candidates.crop(MAX_CANDIDATES_SIZE);

    fastData->FilterCandidatesByResponseBanlist(&candidates);
    FilterRepeatsByReplyHash(sessionState, &candidates);
    if (!isProactivityAllowed) {
        FilterProactivity(&candidates);
    }
    if (requestWrapper.HasExpFlag(EXP_HW_ENABLE_GC_SOFT_REPEATS_FILTER)) {
        if (auto embeder = resources.GetEmbedder(NLU_SEARCH_MODEL_NAME)) {
            const TFilterByEmbeddingModel filterModel(*embeder, NLU_SEARCH_MODEL_NAME, REPEAT_FILTER_BORDER);
            filterModel.FilterCandidates(candidates, sessionState);
        } else {
            candidates.clear();
        }
    }
    if (requestWrapper.HasExpFlag(EXP_HW_GC_DEBUG_FILTER_ALL_CANDIDATES)) {
        candidates.clear();
    }

    if (candidates.empty()) {
        return Nothing();
    }

    return ChooseReply(contextWrapper, sessionState, candidates);
}

void PatchReplyWithMovieDiscussQuestion(const TString& frameName, TGeneralConversationRunContextWrapper& contextWrapper, TReplyInfo* replyInfo) {
    const auto& entity = replyInfo->GetEntityInfo().GetEntity();
    Y_ENSURE(entity.GetEntityCase() != TEntity::EntityCase::ENTITY_NOT_SET);

    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["movie_type"] = entity.GetMovie().GetType();
    nlgData.Context["frame"] = frameName;

    const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_movie_question", nlgData);
    replyInfo->SetRenderedText(renderedPhrase.Text);
    replyInfo->SetRenderedVoice(renderedPhrase.Voice);
}

void PatchReplyWithEasterEgg(TGeneralConversationRunContextWrapper& contextWrapper, TReplyInfo* replyInfo) {
    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["sequence_number"] = replyInfo->GetEasterEggReply().GetSequenceNumber();
    nlgData.Context["text_type"] = "reply";
    nlgData.Context["days"] = replyInfo->GetEasterEggReply().GetDays();

    const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_easter_egg_dialog", nlgData);
    replyInfo->SetRenderedText(renderedPhrase.Text);
    replyInfo->SetRenderedVoice(renderedPhrase.Voice);
}

void PatchReplyWithGenerativeTale(TGeneralConversationRunContextWrapper& contextWrapper, TReplyInfo* replyInfo, const TClassificationResult& classificationResult) {
    const auto& taleState = classificationResult.GetReplyInfo().GetGenerativeTaleReply().GetTaleState();
    const auto stage = taleState.GetStage();

    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["stage"] = GetStageName(stage);
    nlgData.Context["tale_name"] = taleState.GetTaleName();
    nlgData.Context["has_tale_name_and_text"] = !taleState.GetTaleName().empty() && !taleState.GetText().empty();
    nlgData.Context["is_logged_in"] = taleState.GetIsLoggedIn();
    nlgData.Context["no_action_frame_received"] = taleState.GetNoActionFrameReceived();

    if (taleState.GetHasObscene()) {
        if (taleState.GetSkipUtterance()) {
            nlgData.Context["obscene_prefix"] = true;
        } else {
            nlgData.Context["obscene_question"] = taleState.GetActiveQuestion() ? taleState.GetActiveQuestion() : TALE_ASK_CHARACTER;
        }
    }

    bool needOnboarding = NeedTalesOnboarding(contextWrapper, taleState);
    nlgData.Context["onboarding"] = needOnboarding;

    const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_generative_tale", nlgData);

    replyInfo->SetRenderedText(renderedPhrase.Text);
    replyInfo->SetRenderedVoice(renderedPhrase.Voice);
    replyInfo->MutableGenerativeTaleReply()->MutableTaleState()->SetHadOnboarding(taleState.GetHadOnboarding() || needOnboarding);
}

void PatchReplyWithGenerativeToast(TGeneralConversationRunContextWrapper& contextWrapper, TReplyInfo* replyInfo) {
    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["intent"] = replyInfo->GetIntent();
    nlgData.Context["generative_toast"] = replyInfo->GetGenerativeToastReply().GetText();

    const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_generative_toast", nlgData);
    replyInfo->SetRenderedText(renderedPhrase.Text);
    replyInfo->SetRenderedVoice(renderedPhrase.Voice);
}

void TryClassifyWithDummyMicrointents(const TGeneralConversationRunContextWrapper& contextWrapper, TReplyInfo* replyInfo) {
    TString intent;
    float score;
    const auto& resources = contextWrapper.Resources();
    resources.GetMicrointentsClassifier()->TryPredictIntent(GetUtterance(contextWrapper.RequestWrapper()), &intent, &score);
    if (intent) {
        replyInfo->SetIntent(intent);
    }
}

void ChooseMicrointentReply(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TSessionState sessionState, TReplyInfo* replyInfo) {
    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};
    nlgData.Context["frame"] = classificationResult.GetRecognizedFrame().GetName();
    nlgData.Context["intent"] = replyInfo->GetIntent();
    nlgData.Context["is_pure_gc"] = sessionState.GetModalModeEnabled();
    nlgData.Context["rendered_text"] = replyInfo->GetRenderedText();
    nlgData.Context["rendered_voice"] = replyInfo->GetRenderedVoice();
    nlgData.Context["alice_birthday"] = contextWrapper.RequestWrapper().HasExpFlag("alice_birthday");
    nlgData.Context["is_child_microintent"] = classificationResult.GetIsChildTalking() || contextWrapper.RequestWrapper().ClientInfo().IsElariWatch() || contextWrapper.RequestWrapper().ContentRestrictionLevel() == EContentSettings::safe;
    nlgData.Context["emotions_enabled"] = !contextWrapper.RequestWrapper().HasExpFlag(EXP_HW_DISABLE_EMOTIONAL_TTS);
    const auto* microintentInfo = contextWrapper.Resources().GetMicrointents().FindPtr(classificationResult.GetReplyInfo().GetIntent());
    if (microintentInfo && microintentInfo->Emotion) {
        nlgData.Context["emotion"] = microintentInfo->Emotion;
    }
    const auto& renderedAnswer = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_GENERIC_STATIC_REPLY,  nlgData);
    replyInfo->SetRenderedText(renderedAnswer.Text);
    replyInfo->SetRenderedVoice(renderedAnswer.Voice);
}

} // namespace

void TGeneralConversationCandidatesHandle::Do(TScenarioHandleContext& ctx) const {
    TGeneralConversationRunContextWrapper contextWrapper(&ctx);
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    const TSessionState sessionState = GetOnlyProtoOrThrow<TSessionState>(ctx.ServiceCtx, STATE_SESSION);
    auto classificationResult = GetOnlyProtoOrThrow<TClassificationResult>(ctx.ServiceCtx, STATE_CLASSIFICATION_RESULT);
    auto& replyInfo = *classificationResult.MutableReplyInfo();

    if (requestWrapper.HasExpFlag(EXP_HW_GC_DEBUG_CANDIDATES_EXCEPTION)) {
        return;
    }

    {
        if (sessionState.GetModalModeEnabled()) {
            const auto pureSlowdown = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_GC_PURE_SLOWDOWN_USEC);
            if (pureSlowdown) {
                usleep(Min(MAX_SLEEP_USEC, *pureSlowdown));
            }
        } else {
            const auto commonSlowdown = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_GC_COMMON_SLOWDOWN_USEC);
            if (commonSlowdown) {
                usleep(Min(MAX_SLEEP_USEC, *commonSlowdown));
            }
        }
    }

    if (classificationResult.GetHasEntitySearchRequest()) {
        if (auto entitySearchResponse = RetireEntityCandidatesResponse(requestWrapper, ctx); entitySearchResponse.Defined()) {
            *replyInfo.MutableEntityInfo()->MutableEntitySearchCache() = std::move(entitySearchResponse.GetRef());
        }
    }

    if (classificationResult.GetHasGenerativeToastRequest()) {
        if (auto seq2seqResponse = RetireReplySeq2SeqCandidatesResponse(ctx); seq2seqResponse.Defined() && !seq2seqResponse->empty()) {
            replyInfo.MutableGenerativeToastReply()->SetText(seq2seqResponse.GetRef()[contextWrapper.Rng().RandomInteger() % seq2seqResponse->size()].GetText());
        }
    }

    if (replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::REPLYSOURCE_NOT_SET) {
        if (auto replyMaybe = GetAggregatedReply(contextWrapper, classificationResult, sessionState); replyMaybe.Defined()) {
            if (GetAggregatedReplySource(replyMaybe.GetRef()) == SOURCE_PROACTIVITY) {
                *replyInfo.MutableProactivityReply()->MutableNlgSearchReply() = replyMaybe.GetRef().GetNlgSearchReply();
            } else {
                *replyInfo.MutableAggregatedReply() = std::move(replyMaybe.GetRef());
            }
        } else {
            TReplyState replyState;
            LOG_INFO(contextWrapper.Logger()) << "Reply state: " << SerializeProtoText(replyState);
            ctx.ServiceCtx.AddProtobufItem(replyState, STATE_REPLY);
            return;
        }
    }

    if (classificationResult.GetUserLanguage() != ELang::L_RUS) {
        replyInfo.SetLanguage(classificationResult.GetUserLanguage());
    }

    switch (replyInfo.GetReplySourceCase()) {
        case TReplyInfo::ReplySourceCase::kAggregatedReply: {
            const auto mockedAnswer = GetExperimentTypedValue<TString>(requestWrapper, sessionState, EXP_HW_GC_MOCKED_REPLY);
            if (mockedAnswer) {
                replyInfo.SetRenderedText(Base64Decode(mockedAnswer.GetRef()));
            } else {
                replyInfo.SetRenderedText(GetAggregatedReplyText(replyInfo.GetAggregatedReply()));
            }
            if (!requestWrapper.HasExpFlag(EXP_HW_DISABLE_EMOJI_CLASSIFIER)) {
                const auto& resources = contextWrapper.Resources();
                const auto replyEmbedding = GetReplyEmbedding(replyInfo.GetAggregatedReply(), resources.GetEmbedder(NLU_SEARCH_MODEL_NAME), TString{NLU_SEARCH_MODEL_NAME});
                const auto contextEmbedding = resources.GetEmbedder(NLU_SEARCH_CONTEXT_MODEL_NAME)->Embed(GetUtterance(requestWrapper));
                const auto emoji = resources.GetEmojiClassifier()->Predict(contextEmbedding, replyEmbedding);
                if (emoji) {
                    replyInfo.MutableGifsAndEmojiInfo()->SetEmoji(emoji.GetRef());
                }
            }
            AddGifCard(contextWrapper, &replyInfo);
            bool allowedCrosspromo = requestWrapper.ClientInfo().IsSmartSpeaker() || requestWrapper.HasExpFlag(EXP_HW_FACTS_CROSSPROMO_ENABLE_NON_QUASAR);
            if (!requestWrapper.HasExpFlag(EXP_HW_FACTS_CROSSPROMO_DISABLE) && !mockedAnswer && allowedCrosspromo) {
                AddPromoFact(contextWrapper, sessionState, &replyInfo);
            }
            break;
        }
        case TReplyInfo::ReplySourceCase::kProactivityReply: {
            const auto& proactivityReply = replyInfo.GetProactivityReply();
            if (proactivityReply.HasNlgSearchReply()) {
                const auto& action = proactivityReply.GetNlgSearchReply().GetAction();
                auto intent = GetProactivityIntentFromSearchAction(requestWrapper, action, sessionState.GetModalModeEnabled(), contextWrapper.Rng());
                replyInfo.SetIntent(std::move(intent));
            }
            const auto& intent = replyInfo.GetIntent();
            if (intent == FRAME_MOVIE_DISCUSS || (intent == FRAME_MOVIE_DISCUSS_SPECIFIC && classificationResult.GetIsChildTalking())) {
                FillEntityType(contextWrapper, classificationResult.GetIsChildTalking(), &replyInfo);
            }
            if (intent == FRAME_MOVIE_DISCUSS_SPECIFIC) {
                FillEntity(contextWrapper, sessionState, &replyInfo);
            }
            PatchReplyWithEntity(contextWrapper, classificationResult.GetRecognizedFrame().GetName(), &replyInfo);
            break;
        }
        case TReplyInfo::ReplySourceCase::kGenericStaticReply: {
            const auto& intent = replyInfo.GetIntent();
            if (intent == INTENT_MOVIE_QUESTION) {
                PatchReplyWithMovieDiscussQuestion(classificationResult.GetRecognizedFrame().GetName(), contextWrapper, &replyInfo);
            } else if (IsEntitySet(replyInfo.GetEntityInfo().GetEntity())) {
                const static TVector<TStringBuf> PATCH_ENTITY_FRAMES = {FRAME_LETS_DISCUSS_SOME_MOVIE, FRAME_I_DONT_KNOW};
                if (FindPtr(PATCH_ENTITY_FRAMES, classificationResult.GetRecognizedFrame().GetName())) {
                    FillEntity(contextWrapper, sessionState, &replyInfo);
                }
                PatchReplyWithEntity(contextWrapper, classificationResult.GetRecognizedFrame().GetName(), &replyInfo);
            } else if (classificationResult.GetRecognizedFrame().GetName() == FRAME_BANLIST) {
                TryClassifyWithDummyMicrointents(contextWrapper, &replyInfo);
            } else if (classificationResult.GetRecognizedFrame().GetName() == FRAME_MICROINTENTS) {
                ChooseMicrointentReply(contextWrapper, classificationResult, sessionState, &replyInfo);
            }
            break;
        }
        case TReplyInfo::ReplySourceCase::kMovieAkinatorReply:
            break;
        case TReplyInfo::ReplySourceCase::kEasterEggReply:
            PatchReplyWithEasterEgg(contextWrapper, &replyInfo);
            break;
        case TReplyInfo::ReplySourceCase::kGenerativeTaleReply:
            PatchReplyWithGenerativeTale(contextWrapper, &replyInfo, classificationResult);
            break;
        case TReplyInfo::ReplySourceCase::kGenerativeToastReply:
            PatchReplyWithGenerativeToast(contextWrapper, &replyInfo);
            break;
        case TReplyInfo::ReplySourceCase::kSeq2SeqReply:
        case TReplyInfo::ReplySourceCase::kNlgSearchReply:
        case TReplyInfo::ReplySourceCase::REPLYSOURCE_NOT_SET: {
            Y_ENSURE(false);
            break;
        }
    }

    if (RequiresSearchSuggests(requestWrapper, &replyInfo, classificationResult)) {
        AddSuggestCandidatesRequest(requestWrapper, replyInfo.GetRenderedText(), GetDialogHistorySize(requestWrapper, sessionState), &ctx);
    }

    if (replyInfo.GetAggregatedReply().HasNlgSearchReply()) {
        auto nlgSearchReply = replyInfo.GetAggregatedReply().GetNlgSearchReply();
        *replyInfo.MutableNlgSearchReply() = std::move(nlgSearchReply);
    }

    TReplyState replyState;
    *replyState.MutableReplyInfo() = std::move(replyInfo);
    LOG_INFO(contextWrapper.Logger()) << "Reply state: " << SerializeProtoText(replyState);
    ctx.ServiceCtx.AddProtobufItem(replyState, STATE_REPLY);
}

} // namespace NAlice::NHollywood::NGeneralConversation
