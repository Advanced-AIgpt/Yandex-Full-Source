#include "cloud_speaker.h"

#include <voicetech/library/proto_api/ttsbackend.pb.h>

using namespace NAlice::NCuttlefish::NTtsUtils;

void TCloudSpeakerWrap::FillGenerateRequest(TTS::Generate& req) {
    req.set_lang(Params.Lang);
    auto* param = req.mutable_voices()->Add();
    param->set_name(Params.Voice);
    param->set_weight(1.0);
}

TString TCloudSpeakerWrap::GetTtsBackendRequestItemType() {
    return "tts_backend_request_cloud_synth";
}
