#pragma once

#include "request.h"
#include <util/generic/string.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS::NPushNotification {
TCallbackDataSchemeHolder GenerateCallbackDataSchemeHolder(TStringBuf uuid, TStringBuf did, TStringBuf uid, TStringBuf clientId, const NSc::TValue& formData = {});
TString CreatePushCallbackData(TStringBuf uuid, TStringBuf did, TStringBuf uid, TStringBuf clientId, const NSc::TValue& formData = {});

}
