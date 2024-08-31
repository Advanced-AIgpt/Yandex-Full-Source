#include "frame_classifier.h"

#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/generative_tale_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/proactivity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/scenarios/suggesters/movie_akinator/response_body_builder.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/utils/nlu_features.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/memento/proto/user_configs.pb.h>

#include <alice/library/logger/logger.h>

#include <util/random/random.h>
#include <util/string/join.h>


namespace NAlice::NHollywood::NGeneralConversation {

namespace {

using TDialogTurns = ::google::protobuf::RepeatedPtrField<NScenarios::TDialogHistoryDataSource::TDialogTurn>;

constexpr ui64 PROACTIVITY_TIMEOUT_MS = 5*60*1000;
constexpr int PROACTIVITY_FORIDDEN_DIALOG_TURNS_COUNT_LESS = 2;
constexpr ui64 PURE_GC_SESSION_TIMEOUT_MS = 5*60*1000;
constexpr ui64 PURE_GC_SESSION_TIMEOUT_MS_QUASAR = 35*1000;
constexpr ui32 EASTER_EGG_SUGGESTS_COUNT_TRIGGER = 2;
constexpr ui64 TALES_MODALITY_TIMEOUT_SECONDS = 2*60;

const TString PROACTIVITY_FORIDDEN_PREV_SCENARIOS = "";
constexpr ui64 ALICE_BIRTH_EPOCH = 1602288000;
const NDatetime::TCivilSecond ALICE_BIRTH_DATE = NDatetime::Convert(TInstant::FromValue(ALICE_BIRTH_EPOCH*1000*1000), NDatetime::GetTimeZone("UTC"));
constexpr float DEFAULT_GC_CLASSIFIER_SCORE_VALUE = 0.0f;

void SetIrrelevantClassificationResult(TClassificationResult& classificationResult) {
    classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_IRRELEVANT));
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
}

void SetIrrelevantLangClassificationResult(const TString& lang, TClassificationResult& classificationResult) {
    classificationResult.MutableError()->SetType(ToString(INTENT_IRRELEVANT_LANG));
    classificationResult.MutableError()->SetMessage(lang);
}

bool IsPureGcSessionValid(const TScenarioRunRequestWrapper& requestWrapper, const TSessionState& sessionState) {
    const auto currentServerTimeMs = GetServerTimeMs(requestWrapper);
    const auto defaultTimeout = requestWrapper.ClientInfo().IsSmartSpeaker() ? PURE_GC_SESSION_TIMEOUT_MS_QUASAR : PURE_GC_SESSION_TIMEOUT_MS;
    const auto pureGcSessionTimeoutMs = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_GC_DEBUG_PURE_GC_SESSION_TIMEOUT);

    return sessionState.GetLastRequestServerTimeMs() + pureGcSessionTimeoutMs.GetOrElse(defaultTimeout) > currentServerTimeMs;
}

bool IsPrevScenarioAllowedForProactivity(const TScenarioRunRequestWrapper& requestWrapper, const TDialogTurns& dialogTurns, int prevNumber, ui64 prevScenarioTimeout) {
    Y_ENSURE(prevNumber == 1 || prevNumber == 2);
    if (dialogTurns.size() < prevNumber) {
        return true;
    }
    const auto& prevDialogTurn = dialogTurns[dialogTurns.size() - prevNumber];

    const auto& prevTimeoutFlag = prevNumber == 1 ? EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV_SCENARIO_TIMEOUT : EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV2_SCENARIO_TIMEOUT;
    const auto forbiddenPrevScenarioTimeoutMs = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), prevTimeoutFlag).GetOrElse(prevScenarioTimeout);
    if (prevDialogTurn.GetServerTimeMs() + forbiddenPrevScenarioTimeoutMs <= GetServerTimeMs(requestWrapper)) {
        return false;
    }

    const auto& prevScenariosFlag = prevNumber == 1 ? EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV_SCENARIOS : EXP_HW_GC_PROACTIVITY_FORIDDEN_PREV2_SCENARIOS;
    const auto forbiddenPrevScenariosString = GetExperimentTypedValue<TString>(requestWrapper.ExpFlags(), prevScenariosFlag).GetOrElse(PROACTIVITY_FORIDDEN_PREV_SCENARIOS);
    const THashSet<TStringBuf> forbiddenPrevScenarios(StringSplitter(forbiddenPrevScenariosString).Split(';'));
    if (forbiddenPrevScenarios.contains(prevDialogTurn.GetScenarioName())) {
        return false;
    }

    return true;
}

bool IsProactivityDialogHistoryAllowed(const TScenarioRunRequestWrapper& requestWrapper) {
    const auto* dialogHistoryDataSource = requestWrapper.GetDataSource(EDataSourceType::DIALOG_HISTORY);
    Y_ENSURE(dialogHistoryDataSource, "DialogHistoryDataSource not found in the run request");

    const auto& dialogTurns = dialogHistoryDataSource->GetDialogHistory().GetDialogTurns();

    const auto forbiddenPrevDialogTurnsCountLess = GetExperimentTypedValue<int>(requestWrapper.ExpFlags(), EXP_HW_GC_PROACTIVITY_FORIDDEN_DIALOG_TURNS_COUNT_LESS).GetOrElse(PROACTIVITY_FORIDDEN_DIALOG_TURNS_COUNT_LESS);
    if (dialogTurns.size() < forbiddenPrevDialogTurnsCountLess) {
        return false;
    }

    if (!IsPrevScenarioAllowedForProactivity(requestWrapper, dialogTurns, 1, 180000)) {
        return false;
    }

    if (!IsPrevScenarioAllowedForProactivity(requestWrapper, dialogTurns, 2, 240000)) {
        return false;
    }

    return true;
}

bool IsProactivityTimeAllowed(const TScenarioRunRequestWrapper& requestWrapper, const TSessionState& sessionState) {
    if (IsEntitySet(sessionState.GetEntityDiscussion().GetEntity())) {
        return false;
    }
    const auto proactivityTimeoutMs = GetExperimentTypedValue<ui64>(requestWrapper.ExpFlags(), EXP_HW_GC_PROACTIVITY_TIMEOUT).GetOrElse(PROACTIVITY_TIMEOUT_MS);
    const auto currentServerTimeMs = GetServerTimeMs(requestWrapper);

    return sessionState.GetLastProactivityRequestServerTimeMs() + proactivityTimeoutMs <= currentServerTimeMs;
}

bool IsEntitySearchRequestAllowed(const TScenarioRunRequestWrapper& requestWrapper, const TSessionState& sessionState) {
    if (!IsMovieDisscussionAllowedByDefault(requestWrapper, sessionState.GetModalModeEnabled()) && !requestWrapper.HasExpFlag(EXP_HW_GC_PROACTIVITY_ENTITY_SEARCH)) {
        return false;
    }
    if (sessionState.HasEntitySearchCache()) {
        return false;
    }
    const auto uid = GetUid(requestWrapper);
    if (uid.empty()) {
        return false;
    }

    return true;
}

bool IsChildTalking(const TScenarioRunRequestWrapper& requestWrapper) {
    const auto& age = requestWrapper.Proto().GetBaseRequest().GetUserClassification().GetAge();
    return age == NScenarios::TUserClassification::EAge::TUserClassification_EAge_Child;
}

bool CheckTalesBanlist(const TMaybe<TFrame>& callbackFrame, const TScenarioRunRequestWrapper& requestWrapper, const TGeneralConversationFastData* fastData) {
    for (const auto& frameName : WIZ_DETECTION_FRAMES) {
        const TMaybe<TFrame> frameWiz = TryGetFrame(frameName, callbackFrame, requestWrapper.Input());
        if (frameWiz) {
            return true;
        }
    }
    if (fastData->FilterRequest(GetUtterance(requestWrapper), "gc_tale_request_banlist")) {
        return true;
    }

    return false;
}

bool CheckTalesTimeout(const TScenarioRunRequestWrapper& requestWrapper, const TGenerativeTaleState& taleState) {
    const ui64 previousTime = taleState.GetLastRequestTime();
    const ui64 currentTime = GetClientTimeSeconds(requestWrapper);

    return currentTime > previousTime + TALES_MODALITY_TIMEOUT_SECONDS;
}

bool ProcessCharacter(const TMaybe<TFrame>& frame, TClassificationResult& classificationResult, TGenerativeTaleState& taleState) {
    if (const auto* slots = frame->FindSlots("character"); slots && !slots->empty()) {

        TVector<TString> slotsValues;
        for (const auto& slot : *slots) {
            slotsValues.push_back(slot.Value.AsString());
        }
        const auto character = Capitalize(DeletePrepositions(JoinSeq(" ", slotsValues), POSSESSIVE_PREPOSITIONS));
        const auto characterAcc = WideToUTF8(InflectToAcc(UTF8ToWide(character)));

        taleState.SetCharacter(characterAcc);
        classificationResult.MutableReplyInfo()->MutableGenerativeTaleReply()->SetCharacter(characterAcc);
        classificationResult.SetHasGenerativeTaleRequest(true);
        classificationResult.SetNeedContinue(true);

        return true;
    } else {
        classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_CHARACTER));
        return false;
    }
}

void SetDefaultTaleName(TGenerativeTaleState& taleState) {
    const auto& character = taleState.GetCharacter();
    const auto taleName = character ? TString("Сказка про ") + character : TString("Сказка");
    taleState.SetTaleName(taleName);
}

ELang GetUserLanguage(const TScenarioRunRequestWrapper& requestWrapper) {
    return requestWrapper.Proto().GetBaseRequest().GetUserLanguage();
}

} // namespace

TFrameClassifier::TFrameClassifier(const TGeneralConversationResources& resources, const TGeneralConversationFastData* fastData, const TScenarioRunRequestWrapper& requestWrapper, const TScenario::THandleBase& handle, const TSessionState& sessionState)
    : Resources_(resources)
    , FastData_(fastData)
    , RequestWrapper_(requestWrapper)
    , Handle_(handle)
    , SessionState_(sessionState)
    , CallbackFrame_(GetCallbackFrame(RequestWrapper_.Input().GetCallback()))
{
}

void TFrameClassifier::AddGcSensors(TScenarioHandleContext& ctx, float gcScore, bool isModal, bool isSmartSpeaker) const {
    NMonitoring::TLabels labels = ScenarioLabels(Handle_);
    labels.Add("name", "gc_request_type");
    labels.Add("gc_score", ToString(gcScore));
    labels.Add("is_modal", ToString(isModal));
    labels.Add("is_smart_speaker", ToString(isSmartSpeaker));
    ctx.Ctx.GlobalContext().Sensors().IncRate(labels);
}

bool TFrameClassifier::DecideIsAggregatedRequestEnabled(TScenarioHandleContext& ctx, float gcScore) const {
    float gcScoreThreshold;
    bool isModal = SessionState_.GetModalModeEnabled();
    bool isSmartSpeaker = RequestWrapper_.ClientInfo().IsSmartSpeaker();
    AddGcSensors(ctx, gcScore, isModal, isSmartSpeaker);
    if (isSmartSpeaker) {
        gcScoreThreshold = GetExperimentTypedValue<float>(RequestWrapper_.ExpFlags(), EXP_HW_GC_CLASSIFIER_SCORE_THRESHOLD_SPEAKER_PREFIX).GetOrElse(DEFAULT_GC_CLASSIFIER_SCORE_THRESHOLD_SPEAKER);
    } else {
        gcScoreThreshold = GetExperimentTypedValue<float>(RequestWrapper_.ExpFlags(), EXP_HW_GC_CLASSIFIER_SCORE_THRESHOLD_PREFIX).GetOrElse(DEFAULT_GC_CLASSIFIER_SCORE_THRESHOLD);
    }
    if (gcScore > gcScoreThreshold) {
        return true;
    }
    if (!RequestWrapper_.HasExpFlag(EXP_HW_GC_DISABLE_AGGREGATED_REPLY_IN_MODAL_MODE) && isModal) {
        return true;
    }
    return false;
}

void TFrameClassifier::SetDefaultClassificationResult(TScenarioHandleContext& ctx, TClassificationResult& classificationResult, float gcScore) const {
    classificationResult.SetIsAggregatedRequest(DecideIsAggregatedRequestEnabled(ctx, gcScore));
    classificationResult.SetHasSearchReplyRequest(!RequestWrapper_.HasExpFlag(EXP_HW_GC_DISABLE_NLGSEARCH_REPLY));
    classificationResult.SetHasSeq2SeqReplyRequest(classificationResult.GetIsAggregatedRequest() && !RequestWrapper_.HasExpFlag(EXP_HW_GC_DISABLE_SEQ2SEQ_REPLY));
    classificationResult.SetHasSearchSuggestsRequest(true);
    if (classificationResult.GetIsEntitySearchRequestAllowed()) {
        classificationResult.SetHasEntitySearchRequest(true);
    }
}


bool TFrameClassifier::TryDetectMicrointent(TClassificationResult& classificationResult) const {
    const TMaybe<TFrame> frame = TryGetFrame(FRAME_MICROINTENTS, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }
    const auto microintentName = frame->FindSlot("name");
    if (!microintentName) {
        return false;
    }
    bool isFraudMicrointent = microintentName->Value.AsString() == "fraud_request_do_not_know" || microintentName->Value.AsString() == "fraud_request_cant_do";
    if (isFraudMicrointent && RequestWrapper_.HasExpFlag(EXP_HW_DISABLE_FRAUD_MICROINTENTS)) {
        return false;
    }
    if (microintentName->Value.AsString() == "general_conversation") {
        return false;
    }
    if (microintentName->Value.AsString() == "let_us_talk") {
        return false;
    }

    const auto* microintentInfo = Resources_.GetMicrointents().FindPtr(microintentName->Value.AsString());
    if (!microintentInfo) {
        return false;
    }
    if (!RequestWrapper_.HasExpFlag(EXP_HW_ENABLE_BIRTHDAY_MICROINTENTS) && microintentInfo->IsBirthday) {
        return false;
    }
    bool birthdayAllowed = RequestWrapper_.HasExpFlag(EXP_HW_ENABLE_BIRTHDAY_MICROINTENTS) && microintentInfo->IsBirthday;
    if  (!microintentInfo->IsAllowed && !birthdayAllowed && !isFraudMicrointent && !SessionState_.GetModalModeEnabled() && RequestWrapper_.HasExpFlag(EXP_HW_DISABLE_MICROINTENTS_OUTSIDE_MODAL)) {
        return false;
    }
    if (SessionState_.GetModalModeEnabled() && microintentInfo->IsGcFallback) {
        return false;
    }

    const auto microintentScore = frame->FindSlot("confidence");
    if (!microintentScore) {
        return false;
    }

    const auto score = FromString<double>(microintentScore->Value.AsString());
    float threshold = SessionState_.GetModalModeEnabled() ? microintentInfo->ModalThreshold : microintentInfo->Threshold;
    if (score < threshold) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.MutableReplyInfo()->SetIntent(microintentName->Value.AsString());
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();

    return true;
}

bool TFrameClassifier::TryDetectFeedback(TClassificationResult& classificationResult) const {
    const TMaybe<TFrame> frame = TryGetFrame(FRAME_GC_FEEDBACK, CallbackFrame_, RequestWrapper_.Input());
    if (!frame.Defined()) {
        return false;
    }

    const auto feedback = frame->FindSlot("feedback");
    if (!feedback) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.MutableReplyInfo()->SetIntent(feedback->Value.AsString());
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();

    return true;
}

bool TFrameClassifier::TryDetectProactivity(TScenarioHandleContext& ctx,  TClassificationResult& classificationResult) const {
    return TryDetectFrameProactivity(RequestWrapper_, SessionState_.GetModalModeEnabled(), ctx.Rng, &classificationResult);
}

bool TFrameClassifier::TryDetectPureGcActivate(TClassificationResult& classificationResult) const {
    if ((SessionState_.GetModalModeEnabled() && IsPureGcSessionValid(RequestWrapper_, SessionState_)) ||
        RequestWrapper_.HasExpFlag(EXP_HW_GC_DISABLE_PURE_GC)) {
        return false;
    }

    const TMaybe<TFrame> frame = TryGetFrame(FRAME_PURE_GC_ACTIVATE, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    if (RequestWrapper_.HasExpFlag(EXP_HW_ENABLE_HEAVY_SCENARIO_CLASSIFICATION) && !SessionState_.GetIsHeavyScenario()) {
        SetIrrelevantClassificationResult(classificationResult);
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
    return true;
}

bool TFrameClassifier::TryDetectPureGcDeactivate(TClassificationResult& classificationResult) const {
    if (!SessionState_.GetModalModeEnabled()) {
        return false;
    }
    if (!IsPureGcSessionValid(RequestWrapper_, SessionState_)) {
        classificationResult.SetIgnoresExpectedRequest(true);
        classificationResult.SetIsInvalidModalMode(true);
    }

    const TMaybe<TFrame> frame = TryGetFrame(FRAME_PURE_GC_DEACTIVATE, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
    return true;
}

bool TFrameClassifier::TryDetectBanlist(TClassificationResult& classificationResult) const {
    if (RequestWrapper_.HasExpFlag(EXP_HW_GC_DISABLE_BANLIST)) {
        return false;
    }
    const TMaybe<TFrame> frame = TryGetFrame(FRAME_BANLIST, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    const auto banlistIntent = frame->FindSlot("intent");
    if (!banlistIntent || banlistIntent->Value.AsString() != INTENT_DUMMY) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.MutableReplyInfo()->SetIntent(banlistIntent->Value.AsString());
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();

    return true;
}

bool TFrameClassifier::TryDetectLocalBanlist(TClassificationResult& classificationResult) const {
    if (!FastData_->FilterRequest(GetUtterance(RequestWrapper_))) {
        return false;
    }

    classificationResult.MutableRecognizedFrame()->SetName(TString{FRAME_BANLIST});
    classificationResult.MutableReplyInfo()->SetIntent(TString{INTENT_DUMMY});
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
    return true;
}


bool TFrameClassifier::TryDetectRequestForSpecificMovie(const TStringBuf frameName, IRng& rng, TClassificationResult& classificationResult, bool canAskQuestion) const {
    const TMaybe<TFrame> frame = TryGetFrame(frameName, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    const auto ontoId = frame->FindSlot("film_id");
    if (!ontoId) {
        return false;
    }

    const auto* kpIdPtr = Resources_.GetOntoToKpIdMapping().FindPtr(ontoId->Value.AsString());
    if (!kpIdPtr) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    if (frameName == FRAME_LETS_DISCUSS_SPECIFIC_MOVIE) {
        classificationResult.SetIsFrameFeatured(true);
    }
    classificationResult.SetHasSearchSuggestsRequest(true);
    if (!TrySetEntity(GetMovieEntityKey(*kpIdPtr), RequestWrapper_, Resources_, rng, classificationResult.MutableReplyInfo())) {
        classificationResult.MutableReplyInfo()->MutableEntityInfo()->MutableEntity()->MutableMovie()->SetId(*kpIdPtr);
    }

    const auto questionProb = GetExperimentTypedValue<double>(RequestWrapper_.ExpFlags(), EXP_HW_GC_ENTITY_DISCUSSION_QUESTION_PROB_PREFIX);
    if (canAskQuestion && questionProb && rng.RandomDouble() < *questionProb) {
        classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_MOVIE_QUESTION));
        classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
    } else {
        classificationResult.SetHasSearchReplyRequest(true);
    }

    return true;
}

bool TFrameClassifier::TryDetectRequestForSpecificMusic(const TStringBuf frameName, TClassificationResult& classificationResult) const {
    const TMaybe<TFrame> frame = TryGetFrame(frameName, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    const auto ontoId = frame->FindSlot("music_id");
    if (!ontoId) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.SetHasSearchReplyRequest(true);
    classificationResult.SetHasSearchSuggestsRequest(true);
    classificationResult.MutableReplyInfo()->MutableEntityInfo()->MutableEntity()->MutableMusicBand()->SetId(ontoId->Value.AsString());

    return true;
}

bool TFrameClassifier::TryDetectRequestForSpecificGame(const TStringBuf frameName, TClassificationResult& classificationResult) const {
    const TMaybe<TFrame> frame = TryGetFrame(frameName, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    const auto ontoId = frame->FindSlot("game_id");
    if (!ontoId) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.SetHasSearchReplyRequest(true);
    classificationResult.SetHasSearchSuggestsRequest(true);
    classificationResult.MutableReplyInfo()->MutableEntityInfo()->MutableEntity()->MutableVideoGame()->SetId(ontoId->Value.AsString());

    return true;
}

bool TFrameClassifier::TryDetectEntityDiscussRecognizedEntity(IRng& rng, TClassificationResult& classificationResult) const
{
    return TryDetectRequestForSpecificMovie(FRAME_MOVIE_DISCUSS, rng, classificationResult) ||
            TryDetectRequestForSpecificMovie(FRAME_AKINATOR_MOVIE_DISCUSS, rng, classificationResult, /* canAskQuestion= */ false) ||
            TryDetectRequestForSpecificMovie(FRAME_AKINATOR_MOVIE_DISCUSS_WEAK, rng, classificationResult, /* canAskQuestion= */ false) ||
            TryDetectRequestForSpecificMusic(FRAME_MUSIC_DISCUSS, classificationResult) ||
            TryDetectRequestForSpecificGame(FRAME_GAME_DISCUSS, classificationResult);
}

bool TFrameClassifier::TryDetectMovieDiscussWatched(const TStringBuf frameName, bool canAskQuestion, IRng& rng, TClassificationResult& classificationResult) const {
    if (!IsEntitySet(SessionState_.GetEntityDiscussion().GetEntity())) {
        return false;
    }
    const TMaybe<TFrame> frame = TryGetFrame(frameName, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    const auto questionProb = GetExperimentTypedValue<double>(RequestWrapper_.ExpFlags(), EXP_HW_GC_ENTITY_DISCUSSION_QUESTION_PROB_PREFIX);
    if (canAskQuestion && questionProb && rng.RandomDouble() < *questionProb) {
        classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_MOVIE_QUESTION));
        classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
        *classificationResult.MutableReplyInfo()->MutableEntityInfo()->MutableEntity() = SessionState_.GetEntityDiscussion().GetEntity();
        classificationResult.SetHasSearchSuggestsRequest(true);
    } else {
        classificationResult.SetHasSearchReplyRequest(true);
    }

    return true;
}

bool TFrameClassifier::TryDetectMovieDiscussIDontKnow(TClassificationResult& classificationResult) const {
    const TMaybe<TFrame> frame = TryGetFrame(FRAME_I_DONT_KNOW, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.SetHasSearchSuggestsRequest(true);
    if (classificationResult.GetIsEntitySearchRequestAllowed()) {
        classificationResult.SetHasEntitySearchRequest(true);
    }
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
    classificationResult.MutableReplyInfo()->SetIntent(ToString(FRAME_MOVIE_DISCUSS_SPECIFIC));
    *classificationResult.MutableReplyInfo()->MutableEntityInfo()->MutableEntity() = SessionState_.GetEntityDiscussion().GetEntity();

    return true;
}

bool TFrameClassifier::TryDetectLetsDiscussMovie(IRng& rng, TClassificationResult& classificationResult) const
{
    if (TryDetectRequestForSpecificMovie(FRAME_LETS_DISCUSS_SPECIFIC_MOVIE, rng, classificationResult)) {
        return true;
    }

    const TMaybe<TFrame> frame = TryGetFrame(FRAME_LETS_DISCUSS_SOME_MOVIE, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }
    const auto contentType = frame->FindSlot("content_type");
    if (!contentType) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    classificationResult.SetIsFrameFeatured(true);
    classificationResult.SetHasSearchSuggestsRequest(true);
    if (classificationResult.GetIsEntitySearchRequestAllowed()) {
        classificationResult.SetHasEntitySearchRequest(true);
    }
    classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
    classificationResult.MutableReplyInfo()->MutableEntityInfo()->MutableEntity()->MutableMovie()->SetType(contentType->Value.AsString());

    return true;
}

bool TFrameClassifier::TryDetectMovieAkinator(TClassificationResult& classificationResult) const {
    if (!RequestWrapper_.HasExpFlag(EXP_HW_ENABLE_GC_MOVIE_AKINATOR) || !RequestWrapper_.ClientInfo().IsSearchApp()) {
        return false;
    }

    if (const TMaybe<TFrame> frame = LoadAkinatorSemanticFrame(RequestWrapper_.Input())) {
        classificationResult.SetIsFrameFeatured(frame->Name() == FRAME_MOVIE_AKINATOR);
        classificationResult.MutableReplyInfo()->SetIntent(TString{INTENT_MOVIE_AKINATOR});
        classificationResult.MutableReplyInfo()->MutableMovieAkinatorReply();
        *classificationResult.MutableRecognizedFrame() = frame->ToProto();
        return true;
    }
    return false;
}

bool TFrameClassifier::TryDetectEasterEgg(TClassificationResult& classificationResult) const {
    if (RequestWrapper_.HasExpFlag(EXP_HW_GC_DISABLE_EASTER_EGG_HAPPY_BIRTHDAY)) {
        return false;
    }
    const auto clientTime = GetClientTime(RequestWrapper_);
    if (!clientTime) {
        return false;
    }
    const auto diff = NDatetime::GetCivilDiff(ALICE_BIRTH_DATE, clientTime.GetRef(), NDatetime::ECivilUnit::Day).Value;
    if (diff > 5 || diff < -5) {
        return false;
    }

    ui32 sequenceNumber;
    const TMaybe<TFrame> frame = TryGetFrame(FRAME_EASTER_EGG_SUGGESTS_CLICKER, CallbackFrame_, RequestWrapper_.Input());
    const auto suggestsCountTrigger = GetExperimentTypedValue<ui32>(RequestWrapper_.ExpFlags(), EXP_HW_GC_EASTER_EGG_SUGGESTS_COUNT_TRIGGER).GetOrElse(EASTER_EGG_SUGGESTS_COUNT_TRIGGER);
    if (frame) {
        const auto sequenceNumberSlot = frame->FindSlot("sequence_number");
        if (!sequenceNumberSlot) {
            return false;
        }

        if (const auto sequenceNumberMaybe = sequenceNumberSlot->Value.As<ui32>()) {
            sequenceNumber = sequenceNumberMaybe.GetRef();
        } else {
            return false;
        }

        *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    } else if (SessionState_.GetEasterEggState().GetSuggestsClickCount() >= suggestsCountTrigger) {
        sequenceNumber = 0;
    } else {
        return false;
    }

    classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_EASTER_EGG));
    classificationResult.MutableReplyInfo()->MutableEasterEggReply()->SetSequenceNumber(sequenceNumber);
    classificationResult.MutableReplyInfo()->MutableEasterEggReply()->SetDays(diff);

    return true;
}

bool TFrameClassifier::TryDetectGenerativeTales(TClassificationResult& classificationResult, IRng& rng, TRTLogger& logger) const {
    if (SessionState_.GetModalModeEnabled()) {
        return false;
    }

    const auto activateFrame = TryGetFrame(FRAME_GENERATIVE_TALE, CallbackFrame_, RequestWrapper_.Input());
    const auto characterFrame = TryGetFrame(FRAME_GENERATIVE_TALE_CHARACTER, CallbackFrame_, RequestWrapper_.Input());
    const auto confirmSharingFrame = TryGetFrame(FRAME_GENERATIVE_TALE_CONFIRM_SHARING, CallbackFrame_, RequestWrapper_.Input());
    const auto stopFrame = TryGetFrame(FRAME_GENERATIVE_TALE_STOP, CallbackFrame_, RequestWrapper_.Input());
    const auto continueGeneratingTaleFrame = TryGetFrame(FRAME_GENERATIVE_TALE_CONTINUE_GENERATING_TALE, CallbackFrame_, RequestWrapper_.Input());
    const auto taleNameFrame = TryGetFrame(FRAME_GENERATIVE_TALE_TALE_NAME, CallbackFrame_, RequestWrapper_.Input());
    const auto proactivityDeclineFrame = TryGetFrame(FRAME_PROACTIVITY_DECLINE, CallbackFrame_, RequestWrapper_.Input());
    const auto* callback = RequestWrapper_.Input().GetCallback();
    const auto sendMeMyTaleFrame = TryGetFrame(FRAME_GENERATIVE_TALE_SEND_ME_MY_TALE, CallbackFrame_, RequestWrapper_.Input());
    const bool isLoggedIn = !GetUid(RequestWrapper_).empty();
    const bool canShareTale = isLoggedIn || RequestWrapper_.Interfaces().GetCanRenderDiv2Cards();

    TGenerativeTaleState taleState = SessionState_.GetGenerativeTaleState();
    if (RequestWrapper_.IsNewSession()) {
        taleState.Clear();
    }

    const auto& previousStage = taleState.GetStage();
    auto nextStage = previousStage;

    const bool previousStageIsTerminalOrInitial = (
        IsIn(GENERATIVE_TALE_TERMINAL_STAGES, previousStage) ||
        previousStage == TGenerativeTaleState::Undefined
    );

    if (!activateFrame && !sendMeMyTaleFrame && previousStageIsTerminalOrInitial) {
        return false;
    }

    const auto& exitModalityFrame = TryGetFrame(FRAME_GC_FORCE_EXIT, CallbackFrame_, RequestWrapper_.Input());
    if (exitModalityFrame) {
        *classificationResult.MutableRecognizedFrame() = exitModalityFrame->ToProto();
        classificationResult.MutableReplyInfo()->SetIntent(TString{INTENT_GENERATIVE_TALE_FORCE_EXIT});
        return true;
    }

    if (activateFrame) {
        *classificationResult.MutableRecognizedFrame() = activateFrame->ToProto();
    } else if (sendMeMyTaleFrame) {
        *classificationResult.MutableRecognizedFrame() = sendMeMyTaleFrame->ToProto();
    } else if (CheckTalesTimeout(RequestWrapper_, taleState)) {
        classificationResult.SetIgnoresExpectedRequest(true);
        LOG_INFO(logger) << "Generative tales: exit by timeout";
        return false;
    }

    classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE));

    bool shouldBan = CheckTalesBanlist(CallbackFrame_, RequestWrapper_, FastData_);

    if (activateFrame && !shouldBan) {
        taleState.Clear();
        classificationResult.SetIsFrameFeatured(true);
        bool hasCharacter = ProcessCharacter(activateFrame, classificationResult, taleState);
        if (hasCharacter) {
            nextStage = TGenerativeTaleState::FirstQuestion;
        } else {
            nextStage = TGenerativeTaleState::UndefinedCharacter;
        }
    } else if (IsIn({TGenerativeTaleState::Sharing, TGenerativeTaleState::SharingReask}, previousStage)) {
        if (confirmSharingFrame) {
            classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_CONFIRM_SHARING));
            nextStage = TGenerativeTaleState::SharingAskTaleName;
        } else if (stopFrame || proactivityDeclineFrame) {
            classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_STOP));
            nextStage = TGenerativeTaleState::Stop;
        } else if (continueGeneratingTaleFrame) {
            classificationResult.SetHasGenerativeTaleRequest(true);
            classificationResult.SetNeedContinue(true);
            nextStage = TGenerativeTaleState::UndefinedQuestion;
        } else if (const auto gotSilence = callback && callback->GetName() == SILENCE_CALLBACK) {
            taleState.SetHasSilence(true);
            SetDefaultTaleName(taleState);
            classificationResult.SetHasGenerativeTaleRequest(true);
            classificationResult.SetNeedContinue(true);
            nextStage = TGenerativeTaleState::SharingDone;
        } else {
            taleState.SetNoActionFrameReceived(true);
            if (previousStage == TGenerativeTaleState::SharingReask) {
                if (canShareTale) {
                    classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_CONFIRM_SHARING));
                    nextStage = TGenerativeTaleState::SharingAskTaleName;
                } else {
                    classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_STOP));
                    nextStage = TGenerativeTaleState::Stop;
                }
            } else {
                nextStage = TGenerativeTaleState::SharingReask;
            }
        }
    } else if (previousStage == TGenerativeTaleState::SharingAskTaleName) {
        if (taleNameFrame && taleNameFrame->FindSlot("tale_name") && !shouldBan) {
            classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_TALE_NAME));
            taleState.SetTaleName(Capitalize(taleNameFrame->FindSlot("tale_name")->Value.AsString()));
            taleState.SetGotTaleNameFromUser(true);
        } else {
            SetDefaultTaleName(taleState);
        }

        classificationResult.SetHasGenerativeTaleRequest(true);
        classificationResult.SetNeedContinue(true);
        nextStage = TGenerativeTaleState::SharingDone;
    } else if (shouldBan && !taleState.GetHasObscene()) {
        if (IsIn({TGenerativeTaleState::Undefined, TGenerativeTaleState::UndefinedCharacter}, previousStage)) {
            nextStage = TGenerativeTaleState::UndefinedCharacter;
            classificationResult.SetIsFrameFeatured(true);
            classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_BANLIST_ACTIVATION));
        } else {
            classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_BANLIST));
        }
    } else if (characterFrame && !shouldBan) {
        bool hasCharacter = ProcessCharacter(characterFrame, classificationResult, taleState);
        if (hasCharacter) {
            nextStage = TGenerativeTaleState::FirstQuestion;
        } else {
            nextStage = TGenerativeTaleState::Undefined;
        }
    } else if (sendMeMyTaleFrame && previousStageIsTerminalOrInitial) {
        const auto generativeTaleMementoConfig = GetMementoUserConfig(RequestWrapper_).GetGenerativeTale();
        const bool mementoHasTale = (
            !generativeTaleMementoConfig.GetTaleName().empty() && !generativeTaleMementoConfig.GetTaleText().empty()
        );
        taleState.SetTaleName(generativeTaleMementoConfig.GetTaleName());
        taleState.SetText(generativeTaleMementoConfig.GetTaleText());

        classificationResult.SetHasGenerativeTaleRequest(true);
        classificationResult.SetNeedContinue(mementoHasTale);
        classificationResult.SetIsFrameFeatured(true);
        classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_SEND_ME_MY_TALE));
        nextStage = TGenerativeTaleState::SendMeMyTale;
    } else {
        classificationResult.SetHasGenerativeTaleRequest(true);
        classificationResult.SetNeedContinue(true);
        nextStage = MoveToNextQuestion(previousStage);
    }

    if (TryGetFrame(FRAME_GENERATIVE_TALE_STOP, CallbackFrame_, RequestWrapper_.Input())) {
        classificationResult.SetHasGenerativeTaleRequest(false);
        classificationResult.SetNeedContinue(false);
        nextStage = TGenerativeTaleState::Stop;
    }

    taleState.SetSkipUtterance(false);
    if (shouldBan && taleState.GetHasObscene()) {
        taleState.SetSkipUtterance(true);
        classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_GENERATIVE_TALE_BANLIST_CONTINUE));
    }

    if (nextStage == TGenerativeTaleState::OpenQuestion) {
        taleState.SetOpenQuestions(true);
    }

    if (!taleState.GetAvatarsIdForSharedLink()) {
        taleState.SetAvatarsIdForSharedLink(TALE_AVATARS_IDS[rng.RandomInteger(TALE_AVATARS_IDS.size())]);
    }

    if (IsIn({TGenerativeTaleState::Sharing, TGenerativeTaleState::SharingReask, TGenerativeTaleState::SendMeMyTale}, nextStage)) {
        taleState.SetIsLoggedIn(isLoggedIn);
    }

    taleState.SetStage(nextStage);
    taleState.SetHasObscene(shouldBan);
    taleState.SetLastRequestTime(GetClientTimeSeconds(RequestWrapper_));
    *classificationResult.MutableReplyInfo()->MutableGenerativeTaleReply()->MutableTaleState() = taleState;

    return true;
}

bool TFrameClassifier::TryDetectGenerativeToast(IRng& rng, TClassificationResult& classificationResult) const {
    if (!RequestWrapper_.HasExpFlag(EXP_HW_GC_ENABLE_GENERATIVE_TOAST)) {
        return false;
    }

    const TMaybe<TFrame> frame = TryGetFrame(FRAME_GENERATIVE_TOAST, CallbackFrame_, RequestWrapper_.Input());
    if (!frame) {
        return false;
    }

    *classificationResult.MutableRecognizedFrame() = frame->ToProto();
    const auto slot = frame->FindSlot("topic");
    const auto generativeToastProba = GetExperimentTypedValue<double>(RequestWrapper_.ExpFlags(), EXP_HW_GC_GENERATIVE_TOAST_PROBA);
    const bool allowGenerative = ((RequestWrapper_.ContentRestrictionLevel() == EContentSettings::medium ||
        RequestWrapper_.ContentRestrictionLevel() == EContentSettings::without) &&
        rng.RandomDouble() < generativeToastProba.GetOrElse(1.)) ||
        (slot && slot->IsRequested); // If generative toast was allowed earlier and now the slot is filled
    classificationResult.MutableReplyInfo()->SetIntent(allowGenerative && !slot ?
                                                       ToString(INTENT_GENERATIVE_TOAST_TOPIC) :
                                                       allowGenerative ? ToString(INTENT_GENERATIVE_TOAST) : ToString(INTENT_GENERATIVE_TOAST_EDITORIAL));
    classificationResult.SetIsFrameFeatured(true);
    classificationResult.MutableReplyInfo()->MutableGenerativeToastReply();
    if (slot) {
        if (allowGenerative) {
            classificationResult.SetHasGenerativeToastRequest(true);
            classificationResult.MutableReplyInfo()->MutableGenerativeToastReply()->SetTopic(*slot->Value.As<TString>());
        }
    }
    return true;
}

bool TFrameClassifier::TryDetectViolation(TClassificationResult& classificationResult) const {
    if (!RequestWrapper_.HasExpFlag(EXP_HW_GC_ENABLE_VIOLATION_DETECTION)) {
        return false;
    }

    for (const auto& frameName : WIZ_DETECTION_FRAMES) {
        const TMaybe<TFrame> frame = TryGetFrame(frameName, CallbackFrame_, RequestWrapper_.Input());
        if (!frame) {
            continue;
        }
        *classificationResult.MutableRecognizedFrame() = frame->ToProto();
        classificationResult.MutableReplyInfo()->SetIntent(ToString(INTENT_DUMMY));
        classificationResult.MutableReplyInfo()->MutableGenericStaticReply();
        return true;
    }
    return false;
}

bool TFrameClassifier::TryDetectNotRussian(const ELang lang, TClassificationResult& classificationResult) const {
    if (lang == ELang::L_RUS) {
        return false;
    }
    classificationResult.SetHasSeq2SeqReplyRequest(true);
    classificationResult.SetIsAggregatedRequest(false);
    classificationResult.SetHasSearchReplyRequest(false);
    classificationResult.SetHasSearchSuggestsRequest(false);
    classificationResult.SetHasEntitySearchRequest(false);
    return true;
}

TClassificationResult TFrameClassifier::ClassifyRequest(TScenarioHandleContext& ctx) const {
    TClassificationResult classificationResult;
    classificationResult.SetCurrentRequestServerTimeMs(GetServerTimeMs(RequestWrapper_));
    classificationResult.SetCurrentRequestSequenceNumber(SessionState_.GetLastRequestSequenceNumber() + 1);
    classificationResult.SetIsProactivityTimeAllowed(IsProactivityTimeAllowed(RequestWrapper_, SessionState_));
    classificationResult.SetIsProactivityDialogHistoryAllowed(IsProactivityDialogHistoryAllowed(RequestWrapper_));
    classificationResult.SetIsEntitySearchRequestAllowed(IsEntitySearchRequestAllowed(RequestWrapper_, SessionState_));
    classificationResult.SetIsChildTalking(IsChildTalking(RequestWrapper_));

    const auto gcScore = GetNluFeatureValue(RequestWrapper_, NNluFeatures::ENluFeature::AliceGcDssmClassifier).GetOrElse(DEFAULT_GC_CLASSIFIER_SCORE_VALUE);
    classificationResult.SetGcClassifierScore(gcScore);
    classificationResult.SetIsAggregatedRequest(false); // may be overridden later

    if (RequestWrapper_.HasExpFlag(EXP_HW_DISABLE_GC_PROTOCOL)) {
        SetIrrelevantClassificationResult(classificationResult);
        return classificationResult;
    }

    const auto userLanguage = GetUserLanguage(RequestWrapper_);
    classificationResult.SetUserLanguage(userLanguage);
    if (userLanguage != ELang::L_RUS && userLanguage != ELang::L_ARA) {
        SetIrrelevantLangClassificationResult(RequestWrapper_.ClientInfo().Lang, classificationResult);
        return classificationResult;
    }

    if (TryDetectNotRussian(userLanguage, classificationResult)) {
        return classificationResult;
    }
    if (TryDetectFeedback(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectGenerativeTales(classificationResult, ctx.Rng, ctx.Ctx.Logger())) {
        return classificationResult;
    }
    if (TryDetectPureGcActivate(classificationResult)) {
        return classificationResult;
    }
    if (RequestWrapper_.HasExpFlag(EXP_HW_ENABLE_HEAVY_SCENARIO_CLASSIFICATION) && SessionState_.GetIsHeavyScenario() && !SessionState_.GetModalModeEnabled()) {
        SetIrrelevantClassificationResult(classificationResult);
        return classificationResult;
    }
    if (TryDetectPureGcDeactivate(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectMovieAkinator(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectEasterEgg(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectMicrointent(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectBanlist(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectViolation(classificationResult)) {
        return classificationResult;
    }
    if (TryDetectLocalBanlist(classificationResult)) {
        return classificationResult;
    }
    const bool isFrameProactivityAllowed = IsMovieDisscussionAllowedByDefault(RequestWrapper_, SessionState_.GetModalModeEnabled()) || RequestWrapper_.HasExpFlag(EXP_HW_ENABLE_GC_FRAME_PROACTIVITY);
    if (isFrameProactivityAllowed && classificationResult.GetIsProactivityTimeAllowed()) {
        if (TryDetectProactivity(ctx, classificationResult)) {
            return classificationResult;
        }
    }
    if (TryDetectEntityDiscussRecognizedEntity(ctx.Rng, classificationResult)) {
        return classificationResult;
    }
    if (TryDetectMovieDiscussWatched(FRAME_YES_I_WATCH_IT, true, ctx.Rng, classificationResult)) {
        return classificationResult;
    }
    if (TryDetectMovieDiscussWatched(FRAME_NO_I_DID_NOT_WATCH_IT, false, ctx.Rng, classificationResult)) {
        return classificationResult;
    }
    if (TryDetectMovieDiscussIDontKnow(classificationResult)) {
        return classificationResult;
    }
    if (IsMovieDisscussionAllowedByDefault(RequestWrapper_, SessionState_.GetModalModeEnabled()) || RequestWrapper_.HasExpFlag(EXP_HW_GC_LETS_DISCUSS_MOVIE_FRAMES)) {
        if (TryDetectLetsDiscussMovie(ctx.Rng, classificationResult)) {
            return classificationResult;
        }
    }
    if (TryDetectGenerativeToast(ctx.Rng, classificationResult)) {
        return classificationResult;
    }

    SetDefaultClassificationResult(ctx, classificationResult, gcScore);
    return classificationResult;
}

} // namespace NAlice::NHollywood::NGeneralConversation
