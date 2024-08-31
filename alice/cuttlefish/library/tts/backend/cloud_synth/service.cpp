#include "service.h"
#include "cloud_synth.h"

#include <library/cpp/json/json_reader.h>

namespace NAlice::NCloudSynth {
    const TString TService::DefaultConfigResource = "/cloud_synth/default_config.json";
}

using namespace NAlice;
using namespace NAlice::NCloudSynth;

void TService::TRequestProcessor::InitializeTts(TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks) {
    Tts_.Reset(new TCloudSynth(callbacks, Service_.GetCloudSynthConfig(), LogContext_.RtLogPtr()));
}

TService::TService(const TConfig& config)
    : Config_(config)
{
    {
        TStringOutput so(CloudSynthConfig_.BackendUrl_);
        so << Config_.cloud().host() << ":" << Config_.cloud().port();

        CloudSynthConfig_.LegacyOauthToken_ = GetEnv(Config_.cloud().token_var());

        auto voiceToTokenConfigString = GetEnv(Config_.cloud().tokens_config_var());
        NJson::TJsonValue voiceToTokenConfig;
        ReadJsonTree(voiceToTokenConfigString, &voiceToTokenConfig, /* throwOnError */ true);

        for (const auto& [voice, token] : voiceToTokenConfig.GetMap()) {
            CloudSynthConfig_.VoiceToToken_[voice] = token.GetString();
        }

        CloudSynthConfig_.Insecure_ = Config_.cloud().use_insecure_grpc();
    }
}
