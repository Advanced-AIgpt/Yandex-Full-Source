#include "tts_client.h"
#include <alice/cuttlefish/library/tts/backend/base/metrics.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <util/string/builder.h>

using namespace NAlice::NTtsAdapter;

class TTtsClient::TImpl::TTtsHandler: public NVoicetech::TProtobufHandler {
public:
    explicit TTtsHandler(
        TIntrusivePtr<TTtsClient::TImpl>& ttsClientImpl,
        const ::NTts::TBackendRequest& backendRequest,
        TIntrusivePtr<NAlice::NTts::TInterface::TCallbacks> callbacks,
        NAlice::NCuttlefish::TRTLogActivation&& rtLogActivation
    )
        : Metrics_(TTtsHandler::SOURCE_NAME)
        , TtsClientImpl_(ttsClientImpl)
        , BackendRequest_(backendRequest)
        , Callbacks_(callbacks)
        , RtLogActivation_(std::move(rtLogActivation))
    {
        if (backendRequest.HasGenerate()) {
            Serialize(backendRequest.GetGenerate(), RequestData_);
        } else if (backendRequest.HasStopGeneration()) {
            RtLogActivation_.Finish(/* ok = */ false);
            ythrow yexception() << "Stop generation requests not implemented";
        } else {
            RtLogActivation_.Finish(/* ok = */ false);
            ythrow yexception() << "Unknown backend requeset type";
        }

        TtsClientImpl_->RegisterRequest(this, BackendRequest_.GetReqSeqNo());
    }

    ~TTtsHandler() {
        DLOG("~TtsHandler");

        RtLogActivation_.Finish(/* ok = */ !WasError_);
        TtsClientImpl_->OnRequestCompleted(BackendRequest_.GetReqSeqNo(), !WasError_);
    }

protected:
    void OnClose(bool abnormal) override {
        DLOG("handler.OnClose abnormal = " << abnormal);
        if (abnormal) {
            Metrics_.PushRate("closed", "error");
            Metrics_.SetError("abnormalclose");
            OnError("Abnormal close", /* fastError = */ false, /* needCancel = */ false);
        } else {
            Metrics_.PushRate("closed", "ok");
            Closed_ = true;
        }
    }

    void OnError(const NVoicetech::TNetworkError& error) override {
        Metrics_.PushRate("networkerror", ToString(NVoicetech::TNetworkError::TOperation(error.Operation)));
        Metrics_.SetError("networkerror");
        OnError(
            TStringBuilder() << "Network error: " << error.Text() << ", operation = " << error.Operation,
            /* fastError = */ error.Operation == NVoicetech::TNetworkError::OpConnect,
            /* needCancel = */ false
        );
    }

    void OnError(const NVoicetech::TTypedError& error) override {
        Metrics_.PushRate("typederror", ToString(NVoicetech::TTypedError::TType(error.Type)));
        Metrics_.SetError("typederror");
        OnError(TStringBuilder() << "Typed error: " << error.Text << ", type = " << error.Type, /* fastError = */ false, /* needCancel = */ true);
    }

    void OnRecvProtobufError(const TString& error) override {
        Metrics_.SetError("protobuferror");
        OnError(TStringBuilder() << "Recv protobuf error: " << error, /* fastError = */ false, /* needCancel = */ true);
    }

    void OnUpgradeResponse(const THttpParser& parser, const TString& error) override {
        DLOG("handler.OnUpgradeResponse: error = " << error);
        if (Closed_) {
            return;
        }

        if (!error && parser.RetCode() == 101) {
            Metrics_.PushRate("upgraderesponse", "ok");
            RequestUpgraded_ = true;

            Callbacks_->OnRequestProcessingStarted(BackendRequest_.GetReqSeqNo());

            DLOG("handler.Send RequestData");
            auto requestData = std::move(RequestData_);
            RequestData_.clear();
            NVoicetech::TUpgradedHttpHandler::Send(std::move(requestData));
        } else {
            Metrics_.PushRate("upgraderesponseerror", ToString(parser.RetCode()));
            Metrics_.SetError("upgraderesponseerror");

            OnError(
                TStringBuilder() << "Upgrade response error: " << error << ", http_code = " << parser.RetCode(),
                /* fastError = */ parser.RetCode() == 503,
                /* needCancel */ true
            );
        }
    }

    void OnRecvMessage(char* data, size_t size) override {
        DLOG("handler.OnRecvMessage size=" << size);
        if (Closed_) {
            return;
        }

        Metrics_.PushRate(size, "receiveddata", "ok");

        NAlice::NTts::NProtobuf::TBackendResponse backendResponse;
        backendResponse.SetReqSeqNo(BackendRequest_.GetReqSeqNo());
        {
            auto& generateResponse = *backendResponse.MutableGenerateResponse();
            if (!generateResponse.ParseFromArray(data, size)) {
                OnRecvProtobufError("Can not parse protobuf message GenerateResponse");
                return;
            }

            Metrics_.PushRate(generateResponse.audiodata().size(), "receivedaudio", "ok");
        }

        Callbacks_->OnBackendResponse(backendResponse, BackendRequest_.GetGenerate().mime());
    }

private:
    void OnError(const TString& error, bool fastError, bool needCancel) {
        DLOG("handler.OnError: error = " << error << ", fastError = " << fastError << ", needCancel = " << needCancel);
        if (Closed_) {
            return;
        }

        Callbacks_->OnAnyError(error, fastError, BackendRequest_.GetReqSeqNo());
        Closed_ = true;
        WasError_ = true;

        if (needCancel) {
            NVoicetech::TUpgradedHttpHandler::Cancel();
        }
    }

private:
    static constexpr TStringBuf SOURCE_NAME = "tts_handler";
    NTts::TSourceMetrics Metrics_;

    TIntrusivePtr<TTtsClient::TImpl> TtsClientImpl_;
    const ::NTts::TBackendRequest BackendRequest_;
    TVector<TString> RequestData_;
    TIntrusivePtr<NAlice::NTts::TInterface::TCallbacks> Callbacks_;
    NAlice::NCuttlefish::TRTLogActivation RtLogActivation_;

    bool RequestUpgraded_ = false;
    bool Closed_ = false;
    bool WasError_ = false;
};

TString CreateHeaders(const NAlice::NCuttlefish::TRTLogActivation& rtLogActivation) {
    TStringBuilder headers;
    if (rtLogActivation.Token()) {
        headers << "\r\nX-RTLog-Token: " << rtLogActivation.Token();
    }

    return ToString(headers);
}

TTtsClient::TTtsClient(
    const TConfig& config,
    TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks,
    NVoicetech::THttpClient& httpClient,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : Impl_(
        MakeIntrusive<TTtsClient::TImpl>(
            config,
            callbacks,
            httpClient,
            rtLogger
        )
    )
{}

TTtsClient::~TTtsClient() {
    DLOG("~TTtsClient");
}

void TTtsClient::AddBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest) {
    Impl_->AddBackendRequest(backendRequest);
}

void TTtsClient::CancelAll() {
    Impl_->CancelAll();
}

TTtsClient::TTtsClient::TImpl::TImpl(
    const TConfig& config,
    TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks,
    NVoicetech::THttpClient& httpClient,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : Config_(config)
    , Callbacks_(callbacks)
    , HttpClient_(httpClient)
    , RtLogger_(rtLogger)
    , Canceled_(false)
{}

TTtsClient::TImpl::~TImpl() {
    DLOG("~TTtsClient::TImpl");
}


void TTtsClient::TImpl::AddBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest) {
    HttpClient_.GetIOService().Post([self = TIntrusivePtr<TTtsClient::TImpl>(this), backendRequest]() {
        self->AddBackendRequestImpl(backendRequest);
    });
}

void TTtsClient::TImpl::CancelAll() {
    HttpClient_.GetIOService().Post([self = TIntrusivePtr<TTtsClient::TImpl>(this)]() {
        self->CancelAllImpl();
    });
}

void TTtsClient::TImpl::AddBackendRequestImpl(const NTts::NProtobuf::TBackendRequest& backendRequest) {
    if (Canceled_) {
        return;
    }

    if (!AddedReqSeqNo_.insert(backendRequest.GetReqSeqNo()).second) {
        Callbacks_->OnDublicateRequest(backendRequest.GetReqSeqNo());
        return;
    }
    if (!backendRequest.HasGenerate()) {
        Callbacks_->OnInvalidRequest(backendRequest.GetReqSeqNo(), "No generate request in tts backend request");
        return;
    }

    NotStartedRequests_.push_back(backendRequest);
    TryStartNotStartedRequestsProcessing();
}

void TTtsClient::TImpl::CancelAllImpl() {
    if (Canceled_) {
        return;
    }
    Canceled_ = true;

    // WARNING: close first, than cancel
    Callbacks_->OnClosed();

    auto startedRequestsCopy = std::move(StartedRequests_);
    NotStartedRequests_.clear();
    StartedRequests_.clear();

    for (auto& startedRequest : startedRequestsCopy) {
        try {
            startedRequest.Handler_->Cancel();
        } catch (...) {
            Callbacks_->OnAnyError(
                TStringBuilder() << "Failed to cancel request: " << CurrentExceptionMessage(),
                /* fastError = */ false,
                startedRequest.ReqSeqNo_
            );
        }
    }
}

void TTtsClient::TImpl::TryStartNotStartedRequestsProcessing() {
    if (Canceled_) {
        return;
    }

    while (!NotStartedRequests_.empty() && StartedRequests_.size() < Config_.ParallelRequestExecutionLimit_) {
        ui32 requestToStartIndex = 0;
        for (ui32 i = 0; i < NotStartedRequests_.size(); ++i) {
            if (NotStartedRequests_[i].GetReqSeqNo() < NotStartedRequests_[requestToStartIndex].GetReqSeqNo()) {
                requestToStartIndex = i;
            }
        }

        TryStartRequest(requestToStartIndex);
    }
}

void TTtsClient::TImpl::TryStartRequest(ui32 requestIndex) {
    if (Canceled_) {
        return;
    }

    // Keep this object alive until the end of the method execution
    TIntrusivePtr<TTtsClient::TImpl> self(this);

    auto currentRequest = std::move(NotStartedRequests_.at(requestIndex));
    NotStartedRequests_.erase(NotStartedRequests_.begin() + requestIndex);

    ui32 reqSeqNo = currentRequest.GetReqSeqNo();
    Callbacks_->OnStartRequestProcessing(reqSeqNo);

    NAlice::NCuttlefish::TRTLogActivation rtLogActivation =
        RtLogger_
        ? NAlice::NCuttlefish::TRTLogActivation(
            RtLogger_,
            TStringBuilder() << "tts-server-" << reqSeqNo,
            /* newRequest = */ false
        )
        : NAlice::NCuttlefish::TRTLogActivation()
    ;

    try {
        TString headers = CreateHeaders(rtLogActivation);
        DLOG("Headers: " << headers);

        TIntrusivePtr<TTtsHandler> ttsHandler = MakeIntrusive<TTtsHandler>(self, std::move(currentRequest), Callbacks_, std::move(rtLogActivation));
        HttpClient_.RequestUpgrade(Config_.TtsUrl_, NVoicetech::TProtobufHandler::HttpUpgradeType, ttsHandler, headers);
    } catch (...) {
        Callbacks_->OnAnyError(CurrentExceptionMessage(), /* fastError = */ true, reqSeqNo);
        CancelAllImpl();
    }
}

void TTtsClient::TImpl::RegisterRequest(NVoicetech::TProtobufHandler* handler, ui32 reqSeqNo) {
    if (Canceled_) {
        return;
    }

    StartedRequests_.push_back({
        .Handler_ = handler,
        .ReqSeqNo_ = reqSeqNo,
    });
}

void TTtsClient::TImpl::OnRequestCompleted(ui32 reqSeqNo, bool isSuccess) {
    if (Canceled_) {
        return;
    }

    if (!isSuccess) {
        CancelAllImpl();
        return;
    }

    for (ui32 i = 0; i < StartedRequests_.size(); ++i) {
        if (StartedRequests_[i].ReqSeqNo_ == reqSeqNo) {
            StartedRequests_.erase(StartedRequests_.begin() + i);
            TryStartNotStartedRequestsProcessing();
            return;
        }
    }

    Y_VERIFY(false, "Just die to prevent uncontrolled memory leak and dump core file");
}
