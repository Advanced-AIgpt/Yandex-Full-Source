#pragma once

#include <alice/cuttlefish/library/cuttlefish/tts/utils/utils.h>
#include <alice/cuttlefish/library/tts/backend/base/protobuf.h>

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/megamind.pb.h>

#include <voicetech/library/messages/message.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void TtsGenerateMessageToTtsRequest(
        NAlice::NTts::NProtobuf::TRequest& ttsRequest,
        const NAliceProtocol::TRequestContext& requestContext,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NVoicetech::NUniproxy2::TMessage& message
    );

    NVoicetech::NUniproxy2::TMessage CreateTtsGenerate(
        const NAliceProtocol::TRequestContext& requestContext,
        const NAliceProtocol::TMegamindResponse& mmResponse,
        const TString& messageId
    );
}
