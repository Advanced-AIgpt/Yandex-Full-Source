#include "music_match.h"

#include <util/string/builder.h>

using namespace NAlice::NMusicMatch;
using namespace NAlice::NMusicMatchAdapter;
using namespace NAlice::NCuttlefish;

namespace {
    static const TString DEFAULT_USER_AGENT = "music_match_adapter";
    static const TString DEFAULT_AUDIO_FORMAT = "audio/pcm-data";
    static const TString DEFAULT_AUDIO_FORMAT_MIME = "audio/x-pcm;rate=8000";


    TString HttpHeadersToStringForWebsocketRequest(const THttpHeaders& headers) {
        TString headersString;
        TStringOutput stringOutput(headersString);

        // We can't use headers.OutTo here because we need format '\r\nName: Value' not 'Name: Value\r\n'
        for (const auto& header : headers) {
            stringOutput << "\r\n"sv << header.Name() << ": "sv << header.Value();
        }

        return headersString;
    }

} // namespace

TMusicMatch::TMusicMatch(
    TIntrusivePtr<TCallbacks>& callbacks,
    const NCuttlefish::TLogContext& log
)
    : TInterface(callbacks)
    , MusicMatchClient_(new TMyMusicMatchClient(callbacks, log.RtLogPtr()))
    , Log_(log)
{
}

void TMusicMatch::ProcessContextLoadResponse(const NMusicMatch::NProtobuf::TContextLoadResponse& contextLoadResponse) {
    Log_.LogEventInfoCombo<NEvClass::ProcessContextLoadResponse>(contextLoadResponse.ShortUtf8DebugString());
    ContextLoadResponse_ = contextLoadResponse;
}

void TMusicMatch::ProcessSessionContext(const NMusicMatch::NProtobuf::TSessionContext& sessionContext) {
    Log_.LogEventInfoCombo<NEvClass::ProcessSessionContext>(sessionContext.ShortUtf8DebugString());
    SessionContext_ = sessionContext;
}

void TMusicMatch::ProcessTvmServiceTicket(const TString& tvmServiceTicket) {
    Log_.LogEventInfoCombo<NEvClass::ProcessTvmServiceTicket>(tvmServiceTicket);
    TvmServiceTicket_ = tvmServiceTicket;
}

void TMusicMatch::ProcessInitRequest(const NMusicMatch::NProtobuf::TInitRequest& initRequest) {
    Log_.LogEventInfoCombo<NEvClass::ProcessMusicMatchInitRequest>(initRequest.ShortUtf8DebugString());
    InitRequest_ = initRequest;
}

void TMusicMatch::ProcessStreamRequest(const NMusicMatch::NProtobuf::TStreamRequest& streamRequest) {
    if (streamRequest.HasAddData()) {
        auto& addData = streamRequest.GetAddData();
        Log_.LogEventInfoCombo<NEvClass::ProcessMusicMatchStreamRequest>(
            TStringBuilder() << TStringBuf("AddData AudioData.size=") << addData.GetAudioData().size()
        );
    } else {
        Log_.LogEventInfoCombo<NEvClass::ProcessMusicMatchStreamRequest>(streamRequest.ShortUtf8DebugString());
    }
    MusicMatchClient_->SendStreamRequest(streamRequest);
}

void TMusicMatch::CauseError(const TString& error) {
    MusicMatchClient_->SafeCauseError(error);
}

void TMusicMatch::Close() {
    Log_.LogEventInfoCombo<NEvClass::CancelRequest>();
    MusicMatchClient_->SafeClose();
}

void TMusicMatch::SetFinishPromise(NThreading::TPromise<void>& promise) {
    MusicMatchClient_->SetFinishPromise(promise);
}

TString TMusicMatch::InitClientAndGetHeaders() const {
    if (!ContextLoadResponse_.Defined()) {
        ythrow yexception() << "Context load response not defined";
    }
    if (!SessionContext_.Defined()) {
        ythrow yexception() << "Session context not defined";
    }
    if (!TvmServiceTicket_.Defined()) {
        ythrow yexception() << "Tvm service ticket not defined";
    }
    if (!InitRequest_.Defined()) {
        ythrow yexception() << "InitRequest not defined";
    }

    THttpHeaders headers;
    try {
        TStringInput stringInput(InitRequest_->GetHeaders());
        headers = THttpHeaders(&stringInput);
    } catch (...) {
        ythrow yexception() << "Failed to load headers from InitRequest " << CurrentExceptionMessage();
    }

    if (!headers.HasHeader("User-Agent")) {
        headers.AddHeader("User-Agent", DEFAULT_USER_AGENT);
    }

    if (const TString& uuid = SessionContext_->GetUserInfo().GetUuid()) {
        headers.AddHeader("Uniproxy-Uuid", uuid);
    }
    if (const TString& appToken = SessionContext_->GetAppToken()) {
        headers.AddHeader("Uniproxy-Client-Key", appToken);
    }

    if (const TString& audioFormat = InitRequest_->GetAudioFormat(); audioFormat != DEFAULT_AUDIO_FORMAT) {
        Log_.LogEventInfoCombo<NEvClass::CreateAudioConverter>(audioFormat, DEFAULT_AUDIO_FORMAT_MIME);
        MusicMatchClient_->EnableAudioConverter(audioFormat, DEFAULT_AUDIO_FORMAT_MIME);
    }
    headers.AddOrReplaceHeader("Content-Type", DEFAULT_AUDIO_FORMAT);

    headers.AddHeader("X-Ya-Service-Ticket", *TvmServiceTicket_);
    if (const TString& userTicket = ContextLoadResponse_->GetUserTicket()) {
        headers.AddHeader("X-Ya-User-Ticket", userTicket);
    }

    if (const TString& ipAddress = SessionContext_->GetConnectionInfo().GetIpAddress()) {
        headers.AddHeader("X-Forwarded-For", ipAddress);
    }

    if (const TString& rtLogToken = MusicMatchClient_->GetRtLogToken()) {
        // Perhaps it will be implemented in the music proxy sometimes
        headers.AddHeader("X-RTLog-Token", rtLogToken);
    }

    TString stringHeaders = HttpHeadersToStringForWebsocketRequest(headers);
    // WARNING: headers contain secret info
    Log_.LogEventInfo(NEvClass::MusicMatchWebsocketHeaders(stringHeaders));

    return stringHeaders;
}

TMusicMatch::TMyMusicMatchClient::TMyMusicMatchClient(
    TIntrusivePtr<NMusicMatch::TInterface::TCallbacks>& callbacks,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : MusicMatchCallbacks_(callbacks)
{
    if (rtLogger) {
        RtLogChild_ = TRTLogActivation(rtLogger, "music-match-proxy");
    }
}

TString TMusicMatch::TMyMusicMatchClient::GetRtLogToken() const {
    return RtLogChild_.Token();
}

void TMusicMatch::TMyMusicMatchClient::OnInitResponse(const NMusicMatch::NProtobuf::TInitResponse& initResponse) {
    DLOG("client.OnInitResponse: " << initResponse.ShortUtf8DebugString());

    if (!MusicMatchCallbacks_) {
        return;
    }

    MusicMatchCallbacks_->OnInitResponse(initResponse);
    if (!initResponse.GetIsOk()) {
        MusicMatchCallbacks_->OnClosed();
        MusicMatchCallbacks_.Reset();
    }
}

void TMusicMatch::TMyMusicMatchClient::OnStreamResponse(const NMusicMatch::NProtobuf::TStreamResponse& streamResponse) {
    DLOG("client.OnStreamResponse: " << streamResponse.ShortUtf8DebugString());

    if (!MusicMatchCallbacks_) {
        return;
    }

    if (streamResponse.HasMusicResult()) {
        MusicMatchCallbacks_->OnStreamResponse(streamResponse);
        if (!streamResponse.GetMusicResult().GetIsOk()) {
            MusicMatchCallbacks_->OnClosed();
            MusicMatchCallbacks_.Reset();
        }
    } else {
        DLOG("Unexpected TStreamResponse type");
    }
}

void TMusicMatch::TMyMusicMatchClient::OnClosed() {
    DLOG("client.OnClosed");

    if (!MusicMatchCallbacks_) {
        return;
    }

    MusicMatchCallbacks_->OnClosed();
    MusicMatchCallbacks_.Reset();
}

void TMusicMatch::TMyMusicMatchClient::OnAnyError(const TString& error) {
    DLOG("client.OnAnyError: " << error);

    if (RtLogChild_) {
        RtLogChild_.Finish(/* ok= */ false);
    }

    if (!MusicMatchCallbacks_) {
        return;
    }

    MusicMatchCallbacks_->OnAnyError(error);
    MusicMatchCallbacks_.Reset();
}
