#include "repeat_phrase_push.h"

namespace NBASS::NPushNotification {

namespace NQuasarRepeatPhraseActionPush {

namespace {

NSc::TValue GenerateActionPayload(const NSc::TValue& phraseActions) {
    NSc::TValue actionPayload;
    actionPayload["name"] = "update_form";
    actionPayload["type"] = "server_action";
    NSc::TValue phraseSlot;
    phraseSlot["name"] = "phrase_to_repeat";
    phraseSlot["optional"].SetBool(false);
    phraseSlot["type"] = "string";
    phraseSlot["value"] = phraseActions["phrase"];
    NSc::TValue slots;
    slots.Push(phraseSlot);
    actionPayload["payload"]["form_update"]["name"] = QUASAR_IOT_REPEAT_PHRASE;
    actionPayload["payload"]["form_update"]["slots"] = slots;
    actionPayload["payload"]["resubmit"].SetBool(true);
    return actionPayload;
}

}  // namespace


TString GenerateRepeatPhraseActionPayload(const NSc::TValue& phraseActions) {
    NSc::TValue payload = GenerateActionPayload(phraseActions);
    return payload.ToJson();
}

}  // namespace NQuasarRepeatPhraseActionPush

}  // namespace NBASS::NPushNotification