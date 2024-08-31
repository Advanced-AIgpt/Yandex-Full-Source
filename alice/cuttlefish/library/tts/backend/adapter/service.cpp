#include "service.h"

#include "tts.h"

namespace NAlice::NTtsAdapter {
    const TString TService::DefaultConfigResource = "/tts-adapter/default_config.json";
}

using namespace NAlice;
using namespace NAlice::NTtsAdapter;

void TService::TRequestProcessor::InitializeTts(TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks) {
    if (Service_.Config().tts().protocol_version() == NAliceTtsAdapterConfig::Tts::REAL_TTS) {
        Tts_.Reset(new TTts(callbacks, Service_.GetTtsMainClientConfig(), Service_.GetHttpClient(), LogContext_.RtLogPtr()));
    } else if (Service_.Config().tts().protocol_version() == NAliceTtsAdapterConfig::Tts::INTERNAL_FAKE) {
        // Default/base tts service implement fake tts (emulator for testing), so use it
        NTts::TService::TRequestProcessor::InitializeTts(callbacks);
    } else {
        throw yexception() << "for tts_adapter service configured unsupported tts.protocol_version";
    }
}

TService::TService(const TConfig& config)
    : Config_(config)
    , ExecutorsPool_(Config_.tts().client_threads())
{
    {
        // Here we can add options for http client (to tts-server)
        NVoicetech::THttpClientConfig httpClientConfig;
        httpClientConfig.SetConnectTimeout(TFixedDuration(Config_.tts().connect_timeout()));

        HttpClients_.resize(ExecutorsPool_.Size());
        for (size_t i = 0; i < ExecutorsPool_.Size(); ++i) {
            HttpClients_[i].Reset(new NVoicetech::THttpClient(
                httpClientConfig, ExecutorsPool_.GetExecutor().GetIOService(), &ClientsCount_
            ));
        }
    }

    {
        TStringOutput so(TtsMainClientConfig_.TtsUrl_);
        so << TStringBuf("http://") << config.tts().host() << ':' << config.tts().port() << config.tts().path();

        TtsMainClientConfig_.ParallelRequestExecutionLimit_ = config.tts().parallel_request_execution_limit_for_one_apphost_request();
    }
}
