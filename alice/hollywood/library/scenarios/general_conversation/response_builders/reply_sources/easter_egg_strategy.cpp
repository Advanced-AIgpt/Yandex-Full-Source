#include "easter_egg_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/library/logger/logger.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

TEasterEggRenderStrategy::TEasterEggRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyInfo_(replyInfo)
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kEasterEggReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: EasterEggRenderStrategy";
}


void TEasterEggRenderStrategy::AddResponse(TGeneralConversationResponseWrapper* responseWrapper)    {
    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["frame"] = ClassificationResult_.GetRecognizedFrame().GetName();
    nlgData.Context["intent"] = ReplyInfo_.GetIntent();
    nlgData.Context["rendered_text"] = ReplyInfo_.GetRenderedText();
    nlgData.Context["rendered_voice"] = ReplyInfo_.GetRenderedVoice();
    nlgData.Context["alice_birthday"] = ContextWrapper_.RequestWrapper().HasExpFlag("alice_birthday");
    responseWrapper->Builder.GetResponseBodyBuilder()->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_GENERIC_STATIC_REPLY, /* buttons = */ {}, nlgData);

    responseWrapper->SessionState.ClearEasterEggState();
}

void TEasterEggRenderStrategy::AddSuggests(TGeneralConversationResponseWrapper* responseWrapper)    {
    TFrameAction action;
    action.MutableNluHint()->SetFrameName(TString{FRAME_EASTER_EGG_SUGGESTS_CLICKER} + "_hint");
    auto& hintPhrase = *action.MutableNluHint()->AddInstances();
    hintPhrase.SetLanguage(ELang::L_RUS);

    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["sequence_number"] = ReplyInfo_.GetEasterEggReply().GetSequenceNumber();
    nlgData.Context["text_type"] = "suggest";
    nlgData.Context["days"] = ReplyInfo_.GetEasterEggReply().GetDays();
    const auto& suggestPhrase = ContextWrapper_.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_easter_egg_dialog",  nlgData).Text;
    hintPhrase.SetPhrase(suggestPhrase);

    {
        TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
        nlgData.Context["sequence_number"] = ReplyInfo_.GetEasterEggReply().GetSequenceNumber();
        nlgData.Context["text_type"] = "action";
        nlgData.Context["days"] = ReplyInfo_.GetEasterEggReply().GetDays();
        const auto& actionPhrase = ContextWrapper_.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_easter_egg_dialog",  nlgData).Text;

        if (actionPhrase) {
            auto& parsedUtterance = *action.MutableParsedUtterance();
            parsedUtterance.SetUtterance(suggestPhrase);
            auto& musicFrame = *parsedUtterance.MutableFrame();
            musicFrame.SetName("personal_assistant.scenarios.music_play");
            {
                auto& slot = *musicFrame.AddSlots();
                slot.SetName("search_text");
                slot.SetType("string");
                slot.AddAcceptedTypes(slot.GetType());
                slot.SetValue(actionPhrase);
            }
            {
                auto& slot = *musicFrame.AddSlots();
                slot.SetName("action_request");
                slot.SetType("action_request");
                slot.AddAcceptedTypes(slot.GetType());
                slot.SetValue("autoplay");
            }
        } else {
            TSemanticFrame frame;
            frame.SetName(TString{FRAME_EASTER_EGG_SUGGESTS_CLICKER});
            auto& slot = *frame.AddSlots();
            slot.SetName("sequence_number");
            auto& slotValue = *slot.MutableTypedValue();
            slotValue.SetType("string");
            slotValue.SetString(ToString(ReplyInfo_.GetEasterEggReply().GetSequenceNumber() + 1));
            *action.MutableCallback() = ToCallback(frame);
        }
    }

    responseWrapper->Builder.GetResponseBodyBuilder()->AddAction("gc_easter_egg", std::move(action));
    AddSuggest("suggest_gc_easter_egg", suggestPhrase, ToString(SUGGEST_TYPE),
               /* forceGcResponse= */ false, *responseWrapper->Builder.GetResponseBodyBuilder());

}

} // namespace NAlice::NHollywood::NGeneralConversation
