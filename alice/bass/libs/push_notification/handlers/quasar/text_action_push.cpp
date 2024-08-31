#include "text_action_push.h"

namespace NBASS::NPushNotification {

namespace NQuasarTextActionPush {

NSc::TValue GenerateActionPayload(const NSc::TValue& textActions) {
    NSc::TValue push;
    push["to_alice"] = textActions;
    return push;
}

TString GenerateTextActionPayload(const NSc::TValue& textActions) {
    NSc::TValue payload = GenerateActionPayload(textActions);
    return payload.ToJson();
}

} // namespace NQuasarVideoPush

} // namespace NBASS::NPushNotification
