#include "create_callback_data.h"
#include "request.h"

#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

namespace NBASS::NPushNotification {

TCallbackDataSchemeHolder GenerateCallbackDataSchemeHolder(TStringBuf uuid, TStringBuf did, TStringBuf uid, TStringBuf clientId, const NSc::TValue& formData) {
    TCallbackDataSchemeHolder callbackData;
    callbackData->UUId() = uuid;
    callbackData->DId() = did;
    callbackData->UId() = uid;
    callbackData->ClientId() = clientId;
    *callbackData->FormData().GetMutable() = formData;
    return callbackData;
}

TString CreatePushCallbackData(TStringBuf uuid, TStringBuf did, TStringBuf uid, TStringBuf clientId, const NSc::TValue& formData) {
    return GenerateCallbackDataSchemeHolder(uuid, did, uid, clientId, formData)->ToJson();
}

}
