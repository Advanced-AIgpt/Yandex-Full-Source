#include "generative_toast_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/logger/logger.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

TGenerativeToastRenderStrategy::TGenerativeToastRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyInfo_(replyInfo)
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kGenerativeToastReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: GenerativeToastRenderStrategy";
}


void TGenerativeToastRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper) {
    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["frame"] = ClassificationResult_.GetRecognizedFrame().GetName();
    nlgData.Context["intent"] = ReplyInfo_.GetIntent();
    nlgData.Context["rendered_text"] = ReplyInfo_.GetRenderedText();
    nlgData.Context["rendered_voice"] = ReplyInfo_.GetRenderedVoice();
    responseWrapper->Builder.GetResponseBodyBuilder()->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_GENERIC_STATIC_REPLY, /* buttons = */ {}, nlgData);
}

void TGenerativeToastRenderStrategy::AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) {
    if (ReplyInfo_.GetIntent() == INTENT_GENERATIVE_TOAST_TOPIC) {
        const TVector<TString> TOPICS = {"мандарины", "любовь", "здоровье", "чудо", "деньги", "мир", "юмор", "мотивация"};
        for (size_t i = 0; i < TOPICS.size(); ++i) {
            AddSuggest("suggest_generative_toast_topic_" + ToString(i), TOPICS[i], ToString(SUGGEST_TYPE),
                       /* forceGcResponse= */ false, *responseWrapper->Builder.GetResponseBodyBuilder());
        }
    }
    if (ReplyInfo_.GetIntent() != INTENT_GENERATIVE_TOAST) {
        return;
    }
    TFrameAction action;
    action.MutableNluHint()->SetFrameName(TString{FRAME_GENERATIVE_TOAST} + "_hint");
    {
        auto& hintPhrase = *action.MutableNluHint()->AddInstances();
        hintPhrase.SetLanguage(ELang::L_RUS);
        hintPhrase.SetPhrase("еще");
    }

    TSemanticFrame frame;
    frame.SetName(TString{FRAME_GENERATIVE_TOAST});
    *action.MutableCallback() = ToCallback(frame);

    responseWrapper->Builder.GetResponseBodyBuilder()->AddAction("generative_toast", std::move(action));
    AddSuggest("suggest_generative_more", "Еще", ToString(SUGGEST_TYPE),
               /* forceGcResponse= */ false, *responseWrapper->Builder.GetResponseBodyBuilder());
}

TMaybe<TSemanticFrame> TGenerativeToastRenderStrategy::GetSemanticFrame() {
    TSemanticFrame frame;
    frame.SetName(TString{FRAME_GENERATIVE_TOAST});
    if (ReplyInfo_.GetIntent() == INTENT_GENERATIVE_TOAST_TOPIC) {
        auto& requestedSlot = *frame.AddSlots();
        requestedSlot.SetName("topic");
        requestedSlot.AddAcceptedTypes("string");
        requestedSlot.SetIsRequested(true);
    }
    return frame;
}

} // namespace NAlice::NHollywood::NGeneralConversation
