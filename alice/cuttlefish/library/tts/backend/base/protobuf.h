#pragma once

#include <alice/cuttlefish/library/protos/tts.pb.h>

namespace NAlice::NTts {
    namespace NProtobuf {
        using TRequest = ::NTts::TRequest;

        using TBackendRequest = ::NTts::TBackendRequest;
        using TBackendResponse = ::NTts::TBackendResponse;

        using TGenerateRequest = ::TTS::Generate;
        using TGenerateResponse = ::TTS::GenerateResponse;

        using TStopGeneration = ::TTS::StopGeneration;

        using TTimings = ::NTts::TTimings;
    }
}
