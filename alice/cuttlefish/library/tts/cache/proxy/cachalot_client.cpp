#include "cachalot_client.h"
#include "metrics.h"

#include <alice/cuttlefish/library/logging/dlog.h>

#include <util/string/builder.h>

using namespace NAlice::NTtsCacheProxy;

namespace {

class TCachalotHttpHandler : public NVoicetech::THttpHandler {
public:
    TCachalotHttpHandler(
        const TString& key,
        TCachalotClient::ERequestType requestType,
        TIntrusivePtr<NAlice::NTtsCache::TInterface::TCallbacks> callbacks,
        NAlice::NCuttlefish::TRTLogActivation&& rtLogActivation
    )
        : Metrics_(TCachalotHttpHandler::SOURCE_NAMES.at(requestType))
        , StartTime_(TInstant::Now())
        , Key_(key)
        , RequestType_(requestType)
        , Callbacks_(callbacks)
        , RtLogActivation_(std::move(rtLogActivation))
    {}

    ~TCachalotHttpHandler() {
        // Do not PushHist(StartTime_, "duration") here
        // Lifetime of this object is Max(EndTime_ - StartTime_, InputAppHostStreamDuration)
        // Do it in callback
        DLOG("~TCachalotHttpHandler");
    }

    void OnResponse(TAutoPtr<THttpParser> p) override {
        Metrics_.PushHist(StartTime_, "duration", "ok");
        Metrics_.RateHttpCode(p->RetCode(), "cachalot");

        if (p->RetCode() / 100 == 5) {
            Metrics_.SetError("badcode");
            OnError(TStringBuilder() << "Bad cachalot response code " << p->RetCode());
            return;
        }

        Metrics_.PushRate(p->Content().size(), "receivedcontent", "ok");

        NCachalotProtocol::TResponse cachalotResponse;
        if (!cachalotResponse.ParseFromString(p->Content())) {
            Metrics_.SetError("parsecontent");
            OnError(TStringBuilder() << "Failed to parse cachalot response");
            return;
        }

        Metrics_.PushRate("status", NCachalotProtocol::EResponseStatus_Name(cachalotResponse.GetStatus()), "cachalot");

        DLOG("Cachalot response" << cachalotResponse.ShortUtf8DebugString());

        switch (RequestType_) {
            case TCachalotClient::ERequestType::SET: {
                // TODO handle cachalot response statuses
                Callbacks_->OnCacheSetRequestCompleted(Key_, /* error = */ Nothing());
                break;
            }
            case TCachalotClient::ERequestType::GET: {
                NAlice::NTtsCache::NProtobuf::TCacheGetResponse cacheGetResponse;
                cacheGetResponse.SetKey(Key_);

                switch (cachalotResponse.GetStatus()) {
                    case NCachalotProtocol::EResponseStatus::OK: {
                        if (!cacheGetResponse.MutableCacheEntry()->ParseFromString(cachalotResponse.GetGetResp().GetData())) {
                            Metrics_.SetError("parsecacheentry");
                            OnError(TStringBuilder() << "Failed to parse cache entry from cachalot response");
                            return;
                        }

                        Metrics_.PushRate(cacheGetResponse.GetCacheEntry().GetAudio().size(), "receivedaudio", "ok");
                        cacheGetResponse.SetStatus(NAlice::NTtsCache::NProtobuf::ECacheGetResponseStatus::HIT);
                        break;
                    }
                    case NCachalotProtocol::EResponseStatus::NO_CONTENT: {
                        cacheGetResponse.SetStatus(NAlice::NTtsCache::NProtobuf::ECacheGetResponseStatus::MISS);
                        break;
                    }
                    default: {
                        Metrics_.SetError("badcachalotstatus");
                        OnError(TStringBuilder() << "Bad cachalot response status " << NCachalotProtocol::EResponseStatus_Name(cachalotResponse.GetStatus()));
                        return;
                    }
                }

                Callbacks_->OnCacheGetResponse(cacheGetResponse);
                break;
            }
            case TCachalotClient::ERequestType::WARM_UP: {
                // TODO handle cachalot response statuses
                Callbacks_->OnCacheWarmUpRequestCompleted(Key_, /* error = */ Nothing());
                break;
            }
        }
    }

    void OnError(const NVoicetech::TNetworkError& error) override {
        Metrics_.PushHist(StartTime_, "duration", "error");
        Metrics_.PushRate("networkerror", ToString(NVoicetech::TNetworkError::TOperation(error.Operation)));
        Metrics_.SetError("networkerror");
        OnError(TStringBuilder() << "Network error: " << error.Text() << ", operation = " << error.Operation);
    }

    void OnError(const NVoicetech::TTypedError& error) override {
        Metrics_.PushHist(StartTime_, "duration", "error");
        Metrics_.PushRate("typederror", ToString(NVoicetech::TTypedError::TType(error.Type)));
        Metrics_.SetError("typederror");
        OnError(TStringBuilder() << "Typed error: " << error.Text << ", type = " << error.Type);
    }

    void OnStartError(const TString& error) {
        Metrics_.SetError("starterror");
        OnError(TStringBuilder() << "Failed to start cache '" << ToString(RequestType_) << "' request " << error);
    }

private:
    void OnError(const TString& errorMessage) {
        DLOG("TCachalotHttpHandler.OnError: " << errorMessage);

        switch (RequestType_) {
            case TCachalotClient::ERequestType::SET: {
                Callbacks_->OnCacheSetRequestCompleted(Key_, errorMessage);
                break;
            }
            case TCachalotClient::ERequestType::GET: {
                NAlice::NTtsCache::NProtobuf::TCacheGetResponse cacheGetResponse;

                cacheGetResponse.SetKey(Key_);
                cacheGetResponse.SetStatus(NAlice::NTtsCache::NProtobuf::ECacheGetResponseStatus::ERROR);
                cacheGetResponse.SetErrorMessage(errorMessage);

                Callbacks_->OnCacheGetResponse(cacheGetResponse);
                break;
            }
            case TCachalotClient::ERequestType::WARM_UP: {
                Callbacks_->OnCacheWarmUpRequestCompleted(Key_, errorMessage);
                break;
            }
        }

        RtLogActivation_.Finish(/* ok = */ false);
    }

private:
    static constexpr TStringBuf SOURCE_NAME_PREFIX = "cachalot_handler_";
    static inline const THashMap<TCachalotClient::ERequestType, TString> SOURCE_NAMES = {
        {TCachalotClient::ERequestType::SET, SOURCE_NAME_PREFIX + ToString(TCachalotClient::ERequestType::SET)},
        {TCachalotClient::ERequestType::GET, SOURCE_NAME_PREFIX + ToString(TCachalotClient::ERequestType::GET)},
        {TCachalotClient::ERequestType::WARM_UP, SOURCE_NAME_PREFIX + ToString(TCachalotClient::ERequestType::WARM_UP)},
    };
    TSourceMetrics Metrics_;
    TInstant StartTime_;

    TString Key_;
    TCachalotClient::ERequestType RequestType_;
    TIntrusivePtr<NAlice::NTtsCache::TInterface::TCallbacks> Callbacks_;
    NAlice::NCuttlefish::TRTLogActivation RtLogActivation_;
};

using TCachalotHttpHandlerPtr = TIntrusivePtr<TCachalotHttpHandler>;

TString CreateHeaders(
    const TString& key,
    const NAlice::NCuttlefish::TRTLogActivation& rtLogActivation
) {
    TStringBuilder headers;
    headers
        << "\r\nContent-Type: application/protobuf"
        << "\r\nX-Cachalot-Key: " << key
    ;
    if (rtLogActivation.Token()) {
        headers << "\r\nX-RTLog-Token: " << rtLogActivation.Token();
    }

    return ToString(headers);
}

} // namespace

TCachalotClient::TCachalotClient(
    const TCachalotClient::TConfig& config,
    TIntrusivePtr<NTtsCache::TInterface::TCallbacks> callbacks,
    NVoicetech::THttpClient& httpClient,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : Config_(config)
    , Callbacks_(callbacks)
    , HttpClient_(httpClient)
    , RtLogger_(rtLogger)
{}

TCachalotClient::~TCachalotClient() {
    DLOG("~CachalotClient");
}

void TCachalotClient::AddCacheSetRequest(const NTtsCache::NProtobuf::TCacheSetRequest& cacheSetRequest) {
    NCachalotProtocol::TRequest cachalotRequest;
    {
        auto& setRequest = *cachalotRequest.MutableSetReq();
        setRequest.SetKey(cacheSetRequest.GetKey());
        setRequest.SetData(cacheSetRequest.GetCacheEntry().SerializeAsString());
    }

    AddCachalotRequest(cacheSetRequest.GetKey(), ERequestType::SET, cachalotRequest);
}

void TCachalotClient::AddCacheGetRequest(const NTtsCache::NProtobuf::TCacheGetRequest& cacheGetRequest) {
    NCachalotProtocol::TRequest cachalotRequest;
    {
        auto& getRequest = *cachalotRequest.MutableGetReq();
        getRequest.SetKey(cacheGetRequest.GetKey());
    }

    AddCachalotRequest(cacheGetRequest.GetKey(), ERequestType::GET, cachalotRequest);
}

void TCachalotClient::AddCacheWarmUpRequest(const NTtsCache::NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) {
    NCachalotProtocol::TRequest cachalotRequest;
    {
        // Temporary use get request as warm up
        auto& getRequest = *cachalotRequest.MutableGetReq();
        getRequest.SetKey(cacheWarmUpRequest.GetKey());
    }

    AddCachalotRequest(cacheWarmUpRequest.GetKey(), ERequestType::WARM_UP, cachalotRequest);
}

void TCachalotClient::CancelAll() {
    // WARNING: close first, than cancel

    // We can't directly access to callbacks
    // We must use IOService to prevent races
    HttpClient_.GetIOService().Post([callbacks = Callbacks_]() {
        callbacks->OnClosed();
    });

    for (auto httpHandler : HttpHandlers_) {
        // httpHandler->Cancel() directly access to callbacks
        // So we can't call this method from this thread
        // We must use IOService to prevent races
        HttpClient_.GetIOService().Post([httpHandler = httpHandler, callbacks = Callbacks_]() {
            try {
                httpHandler->Cancel();
            } catch (...) {
                callbacks->OnAnyError(TStringBuilder() << "Failed to cancel request: " << CurrentExceptionMessage());
            }
        });
    }
}

TString TCachalotClient::GetUrl(ERequestType requestType) const {
    switch (requestType) {
        case TCachalotClient::SET: {
            return Config_.SetUrl_;
        }
        case TCachalotClient::GET: {
            return Config_.GetUrl_;
        }
        case TCachalotClient::WARM_UP: {
            // Temporary use get request as warm up
            return Config_.GetUrl_;
        }
    }
    Y_UNREACHABLE();
}

void TCachalotClient::AddCachalotRequest(
    const TString& key,
    ERequestType requestType,
    const NCachalotProtocol::TRequest& cachalotRequest
) {
    NAlice::NCuttlefish::TRTLogActivation rtLogActivation =
        RtLogger_
        ? NAlice::NCuttlefish::TRTLogActivation(RtLogger_, TStringBuilder() << "cachalot-" << ToString(requestType) << "-" << ToString(key), /* newRequest = */ false)
        : NAlice::NCuttlefish::TRTLogActivation()
    ;

    TString headers = CreateHeaders(key, rtLogActivation);
    DLOG("Headers: " << headers);

    TIntrusivePtr<TCachalotHttpHandler> httpHandler = MakeIntrusive<TCachalotHttpHandler>(key, requestType, Callbacks_, std::move(rtLogActivation));
    try {
        HttpClient_.RequestPost(GetUrl(requestType), cachalotRequest.SerializeAsString(), httpHandler, headers);
        HttpHandlers_.push_back(httpHandler);
    } catch (...) {
        TString error = CurrentExceptionMessage();

        // We can't directly access to callbacks
        // We must use IOService to prevent races
        HttpClient_.GetIOService().Post([httpHandler = httpHandler, error = std::move(error)]() {
            httpHandler->OnStartError(error);
        });
    }
}
