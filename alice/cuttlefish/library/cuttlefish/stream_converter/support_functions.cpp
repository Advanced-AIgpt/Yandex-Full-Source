#include "support_functions.h"

#include "megamind.h"

#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/library/json/json.h>

using namespace NVoicetech::NUniproxy2;
using namespace NJson;


const TMessage::THeader& NAlice::NCuttlefish::NAppHostServices::NSupport::GetHeaderOrThrow(
    const TMessage& message
) {
    const TMessage::THeader* headerPtr = message.Header.Get();
    if (!headerPtr) {
        ythrow yexception() << "header not found";
    }

    return *headerPtr;
}

const NJson::TJsonValue& NAlice::NCuttlefish::NAppHostServices::NSupport::GetJsonValueByPathOrThrow(
    const NJson::TJsonValue& json,
    const TStringBuf& path
) {
    const NJson::TJsonValue* ptr = json.GetValueByPath(path);
    if (!ptr) {
        ythrow yexception() << path << " not found in json";
    }

    return *ptr;
}
