#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices::NSupport {
    const NVoicetech::NUniproxy2::TMessage::THeader& GetHeaderOrThrow(
        const NVoicetech::NUniproxy2::TMessage& message
    );

    const NJson::TJsonValue& GetJsonValueByPathOrThrow(
        const NJson::TJsonValue& json,
        const TStringBuf& path
    );

}
