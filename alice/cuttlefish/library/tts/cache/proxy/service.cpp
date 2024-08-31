#include "service.h"

#include "tts_cache.h"

namespace NAlice::NTtsCacheProxy {
    const TString TService::DefaultConfigResource = "/tts_cache_proxy/default_config.json";
    TAtomicCounter TService::TRequestProcessor::NextProcNumber_ = 0;
}

using namespace NAlice;
using namespace NAlice::NTtsCacheProxy;

void TService::TRequestProcessor::InitializeTtsCache(TIntrusivePtr<NTtsCache::TInterface::TCallbacks> callbacks) {
    TtsCache_.Reset(new NTtsCacheProxy::TTtsCache(callbacks, Service_.GetCachalotMainClientConfig(), Service_.GetHttpClient(), LogContext_.RtLogPtr()));
}

TIntrusivePtr<NAlice::NTtsCache::TInterface::TCallbacks> TService::TRequestProcessor::CreateTtsCacheCallbacks() {
    return new TTtsCacheCallbacksWithEventlog(RequestHandler_, LogContext_.RtLogPtr(), Number_);
}

TService::TService(const TConfig& config)
    : Config_(config)
    , ExecutorsPool_(Config_.tts_cache().cachalot().client_threads())
    , ClientRequestNum_(0)
{
    // Here we can add options for http client (to cachalot)
    NVoicetech::THttpClientConfig httpClientConfig;
    httpClientConfig.SetConnectTimeout(TFixedDuration(Config_.tts_cache().cachalot().connect_timeout()));

    HttpClients_.resize(ExecutorsPool_.Size());
    for (size_t i = 0; i < ExecutorsPool_.Size(); ++i) {
        HttpClients_[i].Reset(new NVoicetech::THttpClient(
            httpClientConfig, ExecutorsPool_.GetExecutor().GetIOService(), &ClientsCount_
        ));
    }

    {
        TString urlPrefix = TStringBuilder()
            << TStringBuf("http://")
            << config.tts_cache().cachalot().host() << ':'
            << config.tts_cache().cachalot().port()
        ;

        {
            TStringOutput so(CachalotMainClientConfig_.SetUrl_);
            so << urlPrefix << config.tts_cache().cachalot().set_path();
        }

        {
            TStringOutput so(CachalotMainClientConfig_.GetUrl_);
            so << urlPrefix << config.tts_cache().cachalot().get_path();
        }

        {
            TStringOutput so(CachalotMainClientConfig_.WarmUpUrl_);
            so << urlPrefix << config.tts_cache().cachalot().warm_up_path();
        }
    }
}
