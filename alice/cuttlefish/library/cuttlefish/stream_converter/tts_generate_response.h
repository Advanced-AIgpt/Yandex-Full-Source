#pragma once

#include <alice/cuttlefish/library/tts/backend/base/protobuf.h>
#include <voicetech/library/messages/message.h>
#include <voicetech/library/proto_api/ttsbackend.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void TtsGenerateResponseTimingsToJson(
        const TTS::GenerateResponse::Timings& timings,
        bool fromCache,
        NJson::TJsonValue& payload
    );
}
