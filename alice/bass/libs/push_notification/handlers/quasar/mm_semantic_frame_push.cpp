#include "repeat_phrase_push.h"

namespace NBASS::NPushNotification {

namespace NQuasarMMSemanticFrameActionPush {

TString GenerateMMSemanticFrameActionPayload(const NSc::TValue& mmSemanticFramePayload) {
    NSc::TValue actionPayload;
    actionPayload["name"] = "@@mm_semantic_frame";
    actionPayload["type"] = "server_action";
    actionPayload["payload"] = mmSemanticFramePayload;
    return actionPayload.ToJson();
}

}  // namespace NQuasarMMSemanticFrameActionPush

}  // namespace NBASS::NPushNotification
