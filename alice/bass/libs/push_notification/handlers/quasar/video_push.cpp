#include "video_push.h"

namespace NBASS::NPushNotification {

namespace NQuasarVideoPush {

NSc::TValue CreateVideoDescriptor(TStringBuf playUri, TStringBuf provider, TStringBuf providerItemId) {
    NSc::TValue videoDescriptor;

    videoDescriptor["play_uri"] = playUri;
    videoDescriptor["provider_name"] = provider;
    videoDescriptor["provider_item_id"] = providerItemId;

    return videoDescriptor;
}

TString GenerateActionPayload(TStringBuf actionName, const NSc::TValue &videoDescriptor) {
    NSc::TValue form;
    form["name"] = "bass_action";
    form["type"] = "server_action";
    form["payload"]["name"] = actionName;
    form["payload"]["data"]["video_descriptor"] = videoDescriptor;
    return form.ToJson();
}

} // namespace NQuasarVideoPush

} // namespace NBASS::NPushNotification
