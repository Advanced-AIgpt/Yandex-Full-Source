#include "service.h"

#include "unistat.h"
#include "yabio_callbacks_with_eventlog.h"

#undef DLOG
#define DLOG(msg)

namespace NAlice::NYabioAdapter {
    const TString TService::DefaultConfigResource = "/yabio-adapter/default_config.json";
    TAtomicCounter TService::TRequestProcessor::NextProcNumber_ = 0;
}

using namespace NAlice;
using namespace NAlice::NYabioAdapter;
using namespace NAlice::NCuttlefish;

TService::TRequestProcessor::~TRequestProcessor() {
    DLOG("~TRequestProcessor()");
}

TIntrusivePtr<NAlice::NYabio::TInterface::TCallbacks> TService::TRequestProcessor::CreateYabioCallbacks(
    const TString& requestId
) {
    return new TYabioCallbacksWithEventlog(
        RequestHandler_,
        requestId,
        Number_,
        NAlice::NCuttlefish::TLogContext(NCuttlefish::SpawnLogFrame(), Log_.RtLogPtr(), Log_.Options())  // callback work in another thread so has own frame
    );
}

bool TService::TRequestProcessor::OnAppHostProtoItem(
    const TString& type,
    const NAppHost::NService::TProtobufItem& item
) {
    return NYabio::TService::TRequestProcessor::OnAppHostProtoItem(type, item);
}

bool TService::TRequestProcessor::OnCuttlefishAudio(NAliceProtocol::TAudio& audio, bool postponed) {
    if (!postponed) {
        if (audio.HasChunk()) {
            NAliceProtocol::TAudio audioExceptChunk = audio;
            audioExceptChunk.ClearChunk();
            Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
        } else {
            Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
        }
    }
    return NYabio::TService::TRequestProcessor::OnCuttlefishAudio(audio);
}

void TService::TRequestProcessor::OnIgnoreInitRequest(NYabio::NProtobuf::TInitRequest& initRequest, const TString& reason) {
    Log_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "ignore InitRequest: " << reason);
    NYabio::TService::TRequestProcessor::OnIgnoreInitRequest(initRequest, reason);
}

bool TService::TRequestProcessor::OnInitRequest(
    TIntrusivePtr<NYabio::TInterface::TCallbacks> callbacks,
    const TString& requestId
) {
    DLOG("TRequestProcessor::OnInitRequest");
    try {
        OnInitRequestImpl(callbacks, requestId);
    } catch (...) {
        Log_.LogEventErrorCombo<NEvClass::ErrorMessage>(CurrentExceptionMessage());
        // here we has not runned asio thread so can send response directly to apphost context
        NYabio::NProtobuf::TResponse response;
        auto& initResponse = *response.MutableInitResponse();
        NYabio::NProtobuf::FillRequiredDefaults(initResponse);
        initResponse.SetresponseCode(NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR);
        callbacks->OnInitResponse(response);
        callbacks->OnClosed();
        return false;
    }
    return true;
}

void TService::TRequestProcessor::OnInitRequestImpl(
    TIntrusivePtr<NYabio::TInterface::TCallbacks> callbacks,
    const TString& requestId
) {
    if (requestId) {
        Log_.LogEventInfoCombo<NEvClass::RequestId>(requestId);
    }
    if (Service_.Config().yabio().protocol_version() == NAliceYabioAdapterConfig::Yabio::YABIO) {
        // support simple yabio protocol
        DLOG("TRequestProcessor new Yabio_[1] with url=" << Service_.YabioUrl());
        Yabio_.Reset(new TYabio(Service_.GetHttpClient(), Service_.YabioUrl(), callbacks, Log_, NeedResultStep_));
        Yabio_->ProcessInitRequest(InitRequest_);
    } else if (Service_.Config().yabio().protocol_version() == NAliceYabioAdapterConfig::Yabio::INTERNAL_FAKE) {
        // default/base yabio service implement fake yabio (emulator for testing), so use it
        NYabio::TService::TRequestProcessor::OnInitRequest(callbacks, requestId);
    } else {
        throw yexception() << "for yabio_adapter service configured unsupported yabio.protocol_version";
    }
}

bool TService::TRequestProcessor::OnYabioContext(bool hasData, const TString& data) {
    Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostBiometryContext>(data.size());
    return NYabio::TService::TRequestProcessor::OnYabioContext(hasData, data);
}

void TService::TRequestProcessor::OnAsrFinished(const NAliceProtocol::TAsrFinished& asrFinished) {
    Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostAsrFinished>(asrFinished.ShortUtf8DebugString());
    NYabio::TService::TRequestProcessor::OnAsrFinished(asrFinished);
}

void TService::TRequestProcessor::OnAppHostEmptyInput() {
    Log_.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
    //NYabio::TService::TRequestProcessor::OnAppHostEmptyInput();
    if (Yabio_) {
        if (InitRequest_.method() == NAlice::NYabio::NProtobuf::METHOD_SCORE && !HasContextResponse_) {
            Log_.LogEventInfoCombo<NEvClass::InfoMessage>("cancel yabio request (no more results)");
            OnError(NAlice::NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, "yabio_adapter/scoring not receive bio_context_response");
        } else {
            if (!HasEndOfStream_ || RequestHandler_->Context().IsCancelled()) {
                Log_.LogEventInfoCombo<NEvClass::InfoMessage>("yabio_adapter/scoring cancelled (no more results)");
                Yabio_->Close();
            } else {
                Log_.LogEventInfoCombo<NEvClass::InfoMessage>("finalize yabio request");
                Yabio_->SoftClose();
            }
        }
    } else {
        TStringStream ss;
        if (InitRequest_.method() == NAlice::NYabio::NProtobuf::METHOD_SCORE && !HasContextResponse_) {
            ss << "not receive bio_context_response; ";
        }
        if (HasInitRequest_) {
            ss << "break request (return internal error)";
            Log_.LogEventInfoCombo<NEvClass::InfoMessage>(ss.Str());
            SendFastInitResponseError(NAlice::NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR);
        } else {
            ss << "break request (no result)";
            Log_.LogEventInfoCombo<NEvClass::InfoMessage>(ss.Str());
            RequestHandler_->Finish();
        }
    }
}

void TService::TRequestProcessor::OnAppHostClose() {
    Log_.LogEventInfoCombo<NEvClass::ProcessCloseProcessor>();
    NYabio::TService::TRequestProcessor::OnAppHostClose();
    /*
    if (!HasEndOfStream_ || RequestHandler_->Context().IsCancelled()) {
        // lost input (apphost) stream before reach end of stream (intepret it as cancel request)
        DLOG("TRequestProcessor::OnAppHostClose() -> Close() cancel=" << int(RequestHandler_->Context().IsCancelled()) << " y=" << int(bool(Yabio_)));
        if (Yabio_) {
            Yabio_->Close();
        }
    } else {
        // need more time for finish recognition
        if (Yabio_) {
            Yabio_->SoftClose();
        }
        DLOG("TRequestProcessor::OnAppHostClose() -> SoftClose()");
    }
    */
}

void TService::TRequestProcessor::OnWarning(const TString& text) {
    Unistat().OnYabioWarning();
    Log_.LogEventErrorCombo<NEvClass::WarningMessage>(text);
}

void TService::TRequestProcessor::OnError(NAlice::NYabio::NProtobuf::EResponseCode responseCode, const TString& error) {
    Unistat().OnYabioError(responseCode);
    Log_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "code=" << int(responseCode) << ": " << error);
    NYabio::TService::TRequestProcessor::OnError(responseCode, error);
}

void TService::TRequestProcessor::SendFastInitResponseError(NAlice::NYabio::NProtobuf::EResponseCode responseCode) {
    Log_.LogEventErrorCombo<NEvClass::SendToAppHostYabioResponse>("fast InitResponse responseCode=" + ToString(int(responseCode)), /* finalResponse */ true);
    NYabio::TService::TRequestProcessor::SendFastInitResponseError(responseCode);
}

TService::TService(const TConfig& config)
    : Config_(config)
    , ExecutorsPool_(Config_.yabio().client_threads())
{
    // here we can add options for http client (to yabio-server)
    NVoicetech::THttpClientConfig httpClientConfig;
    httpClientConfig.SetConnectTimeout(TFixedDuration(Config_.yabio().connect_timeout()));

    HttpClients_.resize(ExecutorsPool_.Size());
    for (size_t i = 0; i < ExecutorsPool_.Size(); ++i) {
        HttpClients_[i].Reset(new NVoicetech::THttpClient(
            httpClientConfig, ExecutorsPool_.GetExecutor().GetIOService(), &ClientsCount_
        ));
    }
    TStringOutput so(YabioUrl_);
    so << TStringBuf("http://") << config.yabio().host() << ':' << config.yabio().port() << config.yabio().path();
}
