#include "music_match_request.h"
#include "support_functions.h"

#include <library/cpp/http/io/headers.h>

using namespace NAlice::NMusicMatch;
using namespace NJson;
using namespace NVoicetech::NUniproxy2;

bool NAlice::NCuttlefish::NAppHostServices::IsMusicMatchRequestViaAsr(
    const TMessage& message
) {
    return message.Json.GetValueByPath(TStringBuf("event.payload.music_request2")) != nullptr;
}

bool NAlice::NCuttlefish::NAppHostServices::IsRecognizeMusicOnly(
    const TMessage& message
) {
    auto ptr = message.Json.GetValueByPath(TStringBuf("event.payload.recognize_music_only"));
    return ptr != nullptr && ptr->IsBoolean() && ptr->GetBoolean();
}

void NAlice::NCuttlefish::NAppHostServices::VinsMessageToMusicMatchInitRequest(
    const TMessage& message,
    NProtobuf::TInitRequest& initRequest
) {
    const TMessage::THeader& header = NSupport::GetHeaderOrThrow(message);
    const TJsonValue& payload = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload"));

    initRequest.SetRequestId(header.MessageId);

    {
        TString audioFormat;
        GetString(payload, "format", &audioFormat);
        initRequest.SetAudioFormat(audioFormat);
    }
}

void NAlice::NCuttlefish::NAppHostServices::AsrMessageToMusicMatchInitRequest(
    const TMessage& message,
    NProtobuf::TInitRequest& initRequest
) {
    VinsMessageToMusicMatchInitRequest(message, initRequest);

    {
        THttpHeaders headers;
        const TJsonValue& musicRequest2 = NSupport::GetJsonValueByPathOrThrow(message.Json, TStringBuf("event.payload.music_request2"));
        if (!musicRequest2.IsMap()) {
            ythrow yexception() << "unexpected music_request2 format";
        }

        if (musicRequest2.Has("headers")) {
            if (!musicRequest2["headers"].IsMap()) {
                ythrow yexception() << "unexpected music_request2 headrs format";
            }
            for (const auto& [key, value] : musicRequest2["headers"].GetMap()) {
                headers.AddHeader(key, value.GetString());
            }
        }

        TString headersString;
        {
            TStringOutput stringOutput(headersString);
            headers.OutTo(&stringOutput);
        }

        initRequest.SetHeaders(headersString);
    }
}
