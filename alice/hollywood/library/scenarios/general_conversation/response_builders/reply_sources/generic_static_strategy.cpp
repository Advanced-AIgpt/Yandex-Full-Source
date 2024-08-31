#include "generic_static_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/iterator/enumerate.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

const TVector<TStringBuf> FRAMES_WITHOUT_COMMON_SUGGESTS = {FRAME_PURE_GC_ACTIVATE, FRAME_PURE_GC_DEACTIVATE, FRAME_GC_FEEDBACK};
const TVector<TStringBuf> INTENTS_WITHOUT_COMMON_SUGGESTS = {INTENT_GC_PURE_GC_SESSION_TIMEOUT, INTENT_GC_PURE_GC_SESSION_DISABLED, INTENT_IRRELEVANT, INTENT_EASTER_EGG};
const TVector<TStringBuf> INTENTS_PURE_GC_DEACTIVATION = {INTENT_GC_PURE_GC_SESSION_TIMEOUT, INTENT_GC_PURE_GC_SESSION_DISABLED, INTENT_IRRELEVANT};

void AddFeedbackSuggest(TGeneralConversationRunContextWrapper& contextWrapper, const TString& feedbackType, TGeneralConversationResponseWrapper* responseWrapper) {
    TFrameAction action;
    action.MutableNluHint()->SetFrameName(TString{FRAME_GC_FEEDBACK} + "_" + feedbackType);
    auto& hintPhrase = *action.MutableNluHint()->AddInstances();
    hintPhrase.SetLanguage(ELang::L_RUS);
    const auto& suggestPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_suggest_gc_feedback_" + feedbackType,  TNlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()}).Text;
    hintPhrase.SetPhrase(suggestPhrase);

    TSemanticFrame frame;
    frame.SetName(TString{FRAME_GC_FEEDBACK});
    auto& slot = *frame.AddSlots();
    slot.SetName("feedback");
    auto& slotValue = *slot.MutableTypedValue();
    slotValue.SetType("string");
    slotValue.SetString(feedbackType);
    *action.MutableCallback() = ToCallback(frame);

    responseWrapper->Builder.GetResponseBodyBuilder()->AddAction("gc_feedback_" + feedbackType, std::move(action));
    AddSuggest("suggest_gc_feedback_" + feedbackType, suggestPhrase, ToString(SUGGEST_TYPE),
               /* forceGcResponse= */ false, *responseWrapper->Builder.GetResponseBodyBuilder());
}

TVector<TString> SelectFrontalLedImages(const TVector<TVector<TString>>& images, IRng& rng){
    TVector<TString> result;
    result.reserve(images.size());
    for (const auto& imagesSet : images) {
        result.push_back(imagesSet[rng.RandomInteger() % imagesSet.size()]);
    }
    return result;
}

void AskForNextMicrointent(TGeneralConversationRunContextWrapper& contextWrapper, const TString& microintentName, TGeneralConversationResponseWrapper* responseWrapper) {
    const auto* microintentInfo = contextWrapper.Resources().GetMicrointents().FindPtr(microintentName);
    if (!microintentInfo) {
        return;
    }
    TFrameAction action;
    action.MutableNluHint()->SetFrameName(TString{FRAME_MICROINTENTS} + "_");
    for (const auto& phrase : microintentInfo->ActivationPhrases) {
        auto& hintPhrase = *action.MutableNluHint()->AddInstances();
        hintPhrase.SetLanguage(ELang::L_RUS);
        hintPhrase.SetPhrase(phrase);
    }
    TSemanticFrame frame;
    frame.SetName(TString{FRAME_MICROINTENTS});
    auto* slot = frame.AddSlots();
    slot->SetName("name");
    slot->SetType("string");
    slot->SetValue(microintentName);
    slot = frame.AddSlots();
    slot->SetName("confidence");
    slot->SetType("float");
    slot->SetValue("1.0");
    *action.MutableCallback() = ToCallback(frame);

    responseWrapper->Builder.GetResponseBodyBuilder()->AddAction(microintentName, std::move(action));
}

} // namespace

TGenericStaticRenderStrategy::TGenericStaticRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyInfo_(replyInfo)
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kGenericStaticReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: GenericStaticRenderStrategy";
}

void TGenericStaticRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper)    {
    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["frame"] = ClassificationResult_.GetRecognizedFrame().GetName();
    nlgData.Context["intent"] = ReplyInfo_.GetIntent();
    nlgData.Context["rendered_text"] = ReplyInfo_.GetRenderedText();
    nlgData.Context["rendered_voice"] = ReplyInfo_.GetRenderedVoice();
    nlgData.Context["alice_birthday"] = ContextWrapper_.RequestWrapper().HasExpFlag("alice_birthday");
    nlgData.Context["is_child_microintent"] = ClassificationResult_.GetIsChildTalking() || ContextWrapper_.RequestWrapper().ClientInfo().IsElariWatch() ||  ContextWrapper_.RequestWrapper().ContentRestrictionLevel() == EContentSettings::safe;
    nlgData.Context["emotions_enabled"] = !ContextWrapper_.RequestWrapper().HasExpFlag(EXP_HW_DISABLE_EMOTIONAL_TTS);
    if (ReplyInfo_.GetTtsSpeed() != 1.0) {
        nlgData.Context["tts_speed"] = ReplyInfo_.GetTtsSpeed();
    }
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_MICROINTENTS) {
        const auto& interfaces = ContextWrapper_.RequestWrapper().Proto().GetBaseRequest().GetInterfaces();
        const auto& responseBodyBuilder = responseWrapper->Builder.GetResponseBodyBuilder();
        const auto* microintentInfo = ContextWrapper_.Resources().GetMicrointents().FindPtr(ClassificationResult_.GetReplyInfo().GetIntent());
        if (interfaces.GetHasLedDisplay() && microintentInfo && microintentInfo->LedImages) {
            AddFrontalLedImage(SelectFrontalLedImages(microintentInfo->LedImages, ContextWrapper_.Rng()), responseBodyBuilder);
        }
        if (interfaces.GetHasScledDisplay()) {
            AddScledEyes(responseBodyBuilder);
        }
        if (microintentInfo && microintentInfo->Emotion) {
            nlgData.Context["emotion"] = microintentInfo->Emotion;
        }
    }

    responseWrapper->Builder.GetResponseBodyBuilder()->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_GENERIC_STATIC_REPLY, /* buttons = */ {}, nlgData);

    if (IsEntitySet(ReplyInfo_.GetEntityInfo().GetEntity())) {
        UpdateLastDiscussion(ClassificationResult_, &responseWrapper->SessionState);
    }
    if (FindPtr(INTENTS_PURE_GC_DEACTIVATION, ReplyInfo_.GetIntent())) {
        responseWrapper->SessionState.SetModalModeEnabled(false);
    }
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_PURE_GC_ACTIVATE) {
        responseWrapper->SessionState.SetModalModeEnabled(true);
    }
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_PURE_GC_DEACTIVATE) {
        responseWrapper->SessionState.SetModalModeEnabled(false);
    }

    const static TVector<TStringBuf> IRRELEVANT_INTENTS = {INTENT_IRRELEVANT, INTENT_GC_PURE_GC_SESSION_DISABLED};
    if (FindPtr(IRRELEVANT_INTENTS, ReplyInfo_.GetIntent())) {
        responseWrapper->Builder.SetIrrelevant();
    }
}

void TGenericStaticRenderStrategy::AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) {
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_PURE_GC_DEACTIVATE) {
        AddFeedbackSuggest(ContextWrapper_, "positive", responseWrapper);
        AddFeedbackSuggest(ContextWrapper_, "neutral", responseWrapper);
        AddFeedbackSuggest(ContextWrapper_, "negative", responseWrapper);
    }
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_MICROINTENTS) {
        const auto* microintentInfo = ContextWrapper_.Resources().GetMicrointents().FindPtr(ClassificationResult_.GetReplyInfo().GetIntent());
        if (microintentInfo) {
            if (!responseWrapper->SessionState.GetModalModeEnabled()) {
                for (const auto& [suggestNumber, suggest] : Enumerate(microintentInfo->Suggests)) {
                    AddSuggest("suggest_" + ToString(suggestNumber), suggest, ToString(SUGGEST_TYPE),
                            /* forceGcResponse= */ false, *responseWrapper->Builder.GetResponseBodyBuilder());
                }
            }
            for (const auto& nextMicrointentName : microintentInfo->EllipsisIntents) {
                AskForNextMicrointent(ContextWrapper_, nextMicrointentName, responseWrapper);
            }
        }
    }
}

bool TGenericStaticRenderStrategy::NeedCommonSuggests() const {
    if (FindPtr(FRAMES_WITHOUT_COMMON_SUGGESTS, ClassificationResult_.GetRecognizedFrame().GetName())) {
        return false;
    }
    if (FindPtr(INTENTS_WITHOUT_COMMON_SUGGESTS, ReplyInfo_.GetIntent())) {
        return false;
    }

    return true;
}

bool TGenericStaticRenderStrategy::ShouldListen() const {
    if (ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_MICROINTENTS) {
        const auto* microintentInfo = ContextWrapper_.Resources().GetMicrointents().FindPtr(ClassificationResult_.GetReplyInfo().GetIntent());
        if (microintentInfo && microintentInfo->ShouldNotListen) {
            return false;
        }
    }
    if (GC_DUMMY_FRAMES.contains(ClassificationResult_.GetRecognizedFrame().GetName())) {
        return false;
    }
    return true;
}

} // namespace NAlice::NHollywood::NGeneralConversation
