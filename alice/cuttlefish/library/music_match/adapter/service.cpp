#include "service.h"

namespace NAlice::NMusicMatchAdapter {
    const TString TService::DefaultConfigResource = "/music_match_adapter/default_config.json";
    TAtomicCounter TService::TRequestProcessor::NextProcNumber_ = 0;
}

using namespace NAlice;
using namespace NAlice::NMusicMatchAdapter;

bool TService::TRequestProcessor::OnInitRequest(const NMusicMatch::NProtobuf::TInitRequest& initRequest) {
    // WARNING: initRequest.GetHeaders() may contain secret info
    Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostMusicMatchInitRequest>(initRequest.ShortUtf8DebugString());

    if (!NMusicMatch::TService::TRequestProcessor::OnInitRequest(initRequest)) {
        return false;
    }

    try {
        auto* musicMatch = dynamic_cast<NMusicMatchAdapter::TMusicMatch*>(MusicMatch_.Get());
        Service_.StartMusicMatchRequest(Service_.GetHttpClient(), musicMatch->Handler(), musicMatch->InitClientAndGetHeaders());

        return true;
    } catch (...) {
        Log_.LogEventErrorCombo<NEvClass::ErrorMessage>(CurrentExceptionMessage());

        // Here we has not runned asio thread so can send response directly to apphost context
        NMusicMatch::NProtobuf::TInitResponse initResponse;
        initResponse.SetIsOk(false);
        initResponse.SetErrorMessage(CurrentExceptionMessage());

        auto callbacks = MusicMatch_->GetCallbacks();
        callbacks->OnInitResponse(initResponse);
        callbacks->OnClosed();

        return false;
    }
}

void TService::TRequestProcessor::InitializeMusicMatch(TIntrusivePtr<NMusicMatch::TInterface::TCallbacks> callbacks) {
    MusicMatchInitialized_ = true;
    MusicMatch_.Reset(new TMusicMatch(callbacks, Log_));
}

TIntrusivePtr<NAlice::NMusicMatch::TInterface::TCallbacks> TService::TRequestProcessor::CreateMusicMatchCallbacks(NAppHost::TServiceContextPtr& ctx) {
    return new TMusicMatchCallbacksWithEventlog(ctx, Number_);
}

TService::TService(const TConfig& config)
    : Config_(config)
    , ExecutorsPool_(Config_.music_match().client_threads())
{
    // here we can add options for http client (to music stream backend)
    NVoicetech::THttpClientConfig httpClientConfig;
    httpClientConfig.SetConnectTimeout(TFixedDuration(Config_.music_match().connect_timeout()));

    HttpClients_.resize(ExecutorsPool_.Size());
    for (size_t i = 0; i < ExecutorsPool_.Size(); ++i) {
        HttpClients_[i].Reset(new NVoicetech::THttpClient(
            httpClientConfig, ExecutorsPool_.GetExecutor().GetIOService(), &ClientsCount_
        ));
    }

    {
        TStringOutput stringOutput(MusicMatchUrl_);
        stringOutput << TStringBuf("ws://") << config.music_match().host() << ':' << config.music_match().port() << config.music_match().path();
    }
}

void TService::StartMusicMatchRequest(NVoicetech::THttpClient& client, const NVoicetech::TWsHandlerRef& handler, const TString& headers) {
    client.RequestWebSocket(MusicMatchUrl_, handler, headers);
}
