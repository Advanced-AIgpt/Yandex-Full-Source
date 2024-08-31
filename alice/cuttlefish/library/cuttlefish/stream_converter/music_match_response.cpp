#include "music_match_response.h"

using namespace NAlice::NMusicMatch;
using namespace NJson;

void NAlice::NCuttlefish::NAppHostServices::MusicMatchStreamResponseToJson(const NAlice::NMusicMatch::NProtobuf::TStreamResponse& streamResponse, NJson::TJsonValue& payload) {
    if (!streamResponse.GetMusicResult().GetIsOk()) {
        ythrow yexception() << "Can't convert bad music match stream response to json, error: " << streamResponse.GetMusicResult().GetErrorMessage();
    }
    ReadJsonTree(streamResponse.GetMusicResult().GetRawMusicResultJson(), &payload, /*throwOnError = */ true);
}
