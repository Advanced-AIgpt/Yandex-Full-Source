#pragma once

#include "structs.h"

#include <alice/megamind/protos/common/iot.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>


namespace NAlice::NIot {

TString NormalizeWithFST(TStringBuf text, ELanguage language);
TUtf16String PostNormalize(TStringBuf text, ELanguage language);
TString Normalize(TStringBuf text, ELanguage language);

TMaybe<TNluInput> NluInputFromJson(const NSc::TValue& tokens);

bool IsSubType(TStringBuf type, TStringBuf subtype);

bool IsEmpty(const TIoTUserInfo& iotUserInfo);

NSc::TValue LoadAndParseResource(const TStringBuf name);

NAlice::TIoTUserInfo IoTFromIoTValue(const NSc::TValue& iotValue);

} // namespace NAlice::NIot
