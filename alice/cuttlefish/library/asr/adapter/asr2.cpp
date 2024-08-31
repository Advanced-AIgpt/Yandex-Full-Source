#include "asr2.h"

using namespace NAlice::NAsr;
using namespace NAlice::NAsrAdapter;

TAsr2::TAsr2(
    const NProtobuf::TRequest& asrInitRequest,
    TIntrusivePtr<TCallbacks>& callbacks,
    const NCuttlefish::TLogContext& logContextForCallbacks,
    const NCuttlefish::TLogContext& logContext
)
    : TInterface(callbacks)
    , Asr2Client_(new TMyAsr2Client(asrInitRequest, callbacks))
    , CallbacksLog_(logContextForCallbacks)
    , Log_(logContext)
{
    if (Log_.Options().WriteInfoToEventLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfo(NEvClass::DebugMessage(asrInitRequest.ShortUtf8DebugString()));
    }
}

bool TAsr2::ProcessAsrRequest(const NProtobuf::TRequest& request) {
    Asr2Client_->Send(request);
    return true; //TODO:?
}

void TAsr2::CauseError(const TString& error) {
    Asr2Client_->CauseError(error);
}

void TAsr2::Close() {
    Log_.LogEventInfo(NEvClass::CancelRequest());
    Asr2Client_->SafeClose();
}

void TAsr2::SafeInjectAsrResponse(NProtobuf::TResponse&& response) {
    Asr2Client_->GetIOService().Post([client = Asr2Client_, r = std::move(response)]() mutable {
        client->OnAsrResponse(r);
    });
}

TAsr2::TMyAsr2Client::TMyAsr2Client(const NProtobuf::TRequest& asrInitRequest, TIntrusivePtr<TInterface::TCallbacks>& callbacks)
    : TAsr2Client(asrInitRequest)
    , Asr2Callbacks_(callbacks)
{}

void TAsr2::TMyAsr2Client::OnInitResponse(NProtobuf::TResponse& response) {
    if (!Asr2Callbacks_) {
        return;
    }

    Asr2Callbacks_->OnInitResponse(response);
}

void TAsr2::TMyAsr2Client::OnAddDataResponse(NProtobuf::TResponse& response) {
    if (!Asr2Callbacks_) {
        return;
    }

    Asr2Callbacks_->OnAddDataResponse(response);
}

void TAsr2::TMyAsr2Client::OnClosed() {
    if (!Asr2Callbacks_) {
        return;
    }

    Asr2Callbacks_->OnClosed();
    Asr2Callbacks_.Reset();
}

void TAsr2::TMyAsr2Client::OnAnyError(const TString& error, bool fastError) {
    if (!Asr2Callbacks_) {
        return;
    }

    Asr2Callbacks_->OnAnyError(error, fastError);
    Asr2Callbacks_.Reset();
}

void TAsr2::TMyAsr2Client::OnAsrResponse(NProtobuf::TResponse& response) {
    if (!Asr2Callbacks_) {
        return;
    }

    if (response.HasInitResponse()) {
        Asr2Callbacks_->OnInitResponse(response);
        if (!response.GetInitResponse().GetIsOk()) {
            Asr2Callbacks_->OnClosed();
            Asr2Callbacks_.Reset();
        }
    } else if (response.HasAddDataResponse()) {
        Asr2Callbacks_->OnAddDataResponse(response);
        if (!response.GetAddDataResponse().GetIsOk()) {
            Asr2Callbacks_->OnClosed();
            Asr2Callbacks_.Reset();
        }
    } else if (response.HasCloseConnection()) {
        Asr2Callbacks_->OnClosed();
        Asr2Callbacks_.Reset();
    }
}
