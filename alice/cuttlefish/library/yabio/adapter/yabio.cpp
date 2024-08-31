#include "yabio.h"
#include "yabio_callbacks_with_eventlog.h"

#include <util/string/builder.h>

#undef DLOG
#define DLOG(msg)

using namespace NAlice::NYabio;
using namespace NAlice::NYabioAdapter;

TYabio::TYabio(
    NVoicetech::THttpClient& httpClient,
    const TString& url,
    TIntrusivePtr<TCallbacks>& callbacks,
    const NAlice::NCuttlefish::TLogContext& log,
    TDuration needResultStep
)
    : TInterface(callbacks)
    , HttpClient_(httpClient)
    , IOService_(httpClient.GetIOService())
    , Log_(log)
    , YabioUrl_(url)
    , NeedResultStep_(needResultStep)
    , NextNeedResult_(TInstant::Now() + needResultStep)
{
}

bool TYabio::ProcessInitRequest(NProtobuf::TInitRequest& initRequest) {
    if (initRequest.HasOverridePartialUpdatePeriod()) {
        NeedResultStep_ = TDuration::MilliSeconds(initRequest.GetOverridePartialUpdatePeriod());
        NextNeedResult_ = TInstant::Now() + NeedResultStep_;
    }
    if (initRequest.HasDelayMsBeforeFirstPartialResult()) {
        NextNeedResult_ = TInstant::Now() + TDuration::MilliSeconds(initRequest.GetDelayMsBeforeFirstPartialResult());
    }
    YabioClient_.Reset(new TMyYabioClient(initRequest, Callbacks_, Log_.RtLogPtr()));
    Log_.LogEventInfoCombo<NEvClass::YabioRequest>(YabioUrl_);

    TString headers;
    if (Log_.RtLogPtr() && Log_.RtLogPtr()->GetToken()) {
        TStringOutput so(headers);
        so << "\r\nx-rtlog-token: " << Log_.RtLogPtr()->GetToken();
    }

    HttpClient_.RequestUpgrade(YabioUrl_, NVoicetech::TProtobufHandler::HttpUpgradeType, YabioClient_.Get(), headers);
    return true;  // we can return false if don't want continue processing request (and not want use for this throwing exception, which be handler as internal error)
}

bool TYabio::ProcessAddData(NProtobuf::TAddData& addData) {
    auto now = TInstant::Now();
    if (now > NextNeedResult_) {
        addData.SetneedResult(true);
        NextNeedResult_ += NeedResultStep_;
        if (NextNeedResult_ < now) {
            NextNeedResult_ = now + NeedResultStep_;
        }
    }
    YabioClient_->Send(IOService_, addData);
    return true;  // we can return false if don't want continue processing request
}

void TYabio::CauseError(NProtobuf::EResponseCode responseCode, const TString& error) {
    YabioClient_->CauseError(IOService_, responseCode, error);
}

void TYabio::Close() {
    if (!YabioClient_) {
        return;  // request can be finished  without receivig InitRequest (& creating YabioClient)
    }

    Log_.LogEventInfoCombo<NEvClass::CancelRequest>();
    YabioClient_->SafeCancel(IOService_);
}

void TYabio::SoftClose() {
    if (!YabioClient_) {
        return;  // request can be finished  without receivig InitRequest (& creating YabioClient)
    }

    YabioClient_->SoftClose(IOService_);
}

void TYabio::SafeInjectYabioResponse(NProtobuf::TResponse&& response) {

    IOService_.Post([client = YabioClient_, r = std::move(response)]() mutable {
        client->OnYabioResponse(r);
    });
}

TYabio::TMyYabioClient::TMyYabioClient(const NProtobuf::TInitRequest& initRequest, TIntrusivePtr<TInterface::TCallbacks>& callbacks, NRTLog::TRequestLoggerPtr rtLog)
    : TYabioClient(initRequest)
    , YabioCallbacks_(callbacks)
    , CallbacksLog_(dynamic_cast<TYabioCallbacksWithEventlog*>(callbacks.Get())->LogFrame(), rtLog)
{
    GroupId_ = initRequest.group_id();
}

void TYabio::TMyYabioClient::OnSendInitRequest() {
    CallbacksLog_.LogEventInfoCombo<NEvClass::SendToYabioInitRequest>(InitRequestDebugString_);
}

void TYabio::TMyYabioClient::OnSendAddData(size_t size) {
    CallbacksLog_.LogEventInfoCombo<NEvClass::SendToYabioAddData>(size);
}

void TYabio::TMyYabioClient::OnInitResponse(NProtobuf::TResponse& response) {
    bool initFailed = response.GetInitResponse().GetresponseCode() != NYabio::NProtobuf::RESPONSE_CODE_OK;
    if (initFailed) {
        ShutdownSending();  // cancel request
    }
    if (!YabioCallbacks_) {
        return;
    }

    CallbacksLog_.LogEventInfoCombo<NEvClass::RecvFromYabioInitResponse>(response.ShortUtf8DebugString());
    if (GroupId_) {
        response.SetGroupId(GroupId_);
    }
    YabioCallbacks_->OnInitResponse(response);
}

void TYabio::TMyYabioClient::OnAddDataResponse(NProtobuf::TResponse& response) {
    if (!YabioCallbacks_) {
        return;
    }

    NProtobuf::TAddDataResponse addDataResponse{response.GetAddDataResponse()};  // make copy for remove useless data blob
    for (auto& e : *addDataResponse.mutable_context()->mutable_enrolling()) {
        e.clear_voiceprint();
    }
    CallbacksLog_.LogEventInfoCombo<NEvClass::RecvFromYabioAddDataResponse>(addDataResponse.ShortUtf8DebugString());
    YabioCallbacks_->OnAddDataResponse(response);
}

void TYabio::TMyYabioClient::OnClosed() {
    if (!YabioCallbacks_) {
        return;
    }

    CallbacksLog_.LogEventInfoCombo<NEvClass::YabioConnectionClosed>();
    YabioCallbacks_->OnClosed();
    YabioCallbacks_.Reset();
}

void TYabio::TMyYabioClient::OnAnyError(NProtobuf::EResponseCode responseCode, const TString& error, bool fastError) {
    if (!YabioCallbacks_) {
        return;
    }

    CallbacksLog_.LogEventErrorCombo<NEvClass::WarningMessage>(TStringBuilder() << "yabio client error: code=" << int(responseCode) << " fast=" << fastError << ": " << error);
    YabioCallbacks_->OnAnyError(responseCode, error, fastError);
    YabioCallbacks_.Reset();
}

void TYabio::TMyYabioClient::OnYabioResponse(NProtobuf::TResponse& response) {
    if (!YabioCallbacks_) {
        return;
    }
    if (GroupId_) {
        response.SetGroupId(GroupId_);
    }
    if (response.HasInitResponse()) {
        YabioCallbacks_->OnInitResponse(response);
        if (response.GetInitResponse().GetresponseCode() != NProtobuf::RESPONSE_CODE_OK) {
            YabioCallbacks_->OnClosed();
            YabioCallbacks_.Reset();
        }
    } else if (response.HasAddDataResponse()) {
        YabioCallbacks_->OnAddDataResponse(response);
        if (response.GetAddDataResponse().GetresponseCode() != NProtobuf::RESPONSE_CODE_OK) {
            YabioCallbacks_->OnClosed();
            YabioCallbacks_.Reset();
        }
    }
}

