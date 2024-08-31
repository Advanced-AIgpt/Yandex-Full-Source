#include "asr2_client.h"

#include <util/string/builder.h>

#undef DLOG
#define DLOG(msg)

using namespace NAlice::NAsrAdapter;

void TAsr2Client::Send(const NAsr::NProtobuf::TRequest& request) {
    if (Closed_) {
        return;
    }

    bool closeConnection = false;
    if (request.HasCloseConnection()) {
        closeConnection = true;
    }

    TVector<TString> data;
    Serialize(request, data);
    GetIOService().Post([self = TIntrusivePtr(this), data2 = std::move(data), closeConnection]() mutable {
        if (self->Closed_ || self->CloseConnectionSended_) {
            return;
        }

        if (closeConnection) {
            self->CloseConnectionSended_ = true;
        }
        if (!self->HasInitResponse_) {
            self->PostponedRequests_.emplace_back(std::move(data2));
            return;
        }

        DLOG("client.Send AddData");
        self->NVoicetech::TUpgradedHttpHandler::Send(std::move(data2));
    });
}

void TAsr2Client::CauseError(const TString& error) {
    if (Closed_) {
        return;
    }

    GetIOService().Post([self = TIntrusivePtr(this), error = TString(error)]() mutable {
        if (self->Closed_ || self->CloseConnectionSended_) {
            return;
        }

        self->Closed_ = true;
        self->OnAnyError(error);
        self->ShutdownSending();
    });
}

void TAsr2Client::SafeClose() {
    if (Closed_) {
        return;
    }

    GetIOService().Post([self = TIntrusivePtr(this)]() mutable {
        if (self->Closed_) {
            return;
        }

        self->Closed_ = true;
        self->OnClosed();
        self->NVoicetech::TUpgradedHttpHandler::Cancel();
    });
}

void TAsr2Client::OnError(const NVoicetech::TNetworkError& error) {
    if (Closed_) {
        return;
    }

    Closed_ = true;
    bool fastError = error.Operation == NVoicetech::TNetworkError::OpConnect;
    OnAnyError(TStringBuilder() << "network op=" << int(error.Operation) << " errno=" << error.Value() << ": " << error.Text(), fastError);
}

void TAsr2Client::OnError(const NVoicetech::TTypedError& error) {
    if (Closed_) {
        return;
    }

    Closed_ = true;
    OnAnyError(error.Text);
    ShutdownSending();
}

void TAsr2Client::OnRecvProtobufError(const TString& error) {
    DLOG("client.OnRecvProtobufError: " << error);
    if (Closed_) {
        return;
    }

    TIntrusivePtr<TAsr2Client> self(this);
    OnAnyError(error);
    if (!Closed_) {
        Closed_ = true;
        ShutdownSending();
    }
}

void TAsr2Client::OnUpgradeResponse(const THttpParser& p, const TString& error) {
    // used only for client connection
    DLOG("client.OnUpgradeResponse: error=" << error);
    if (!error && p.RetCode() == 101) {
        DLOG("client.Send InitRequest");
        NVoicetech::TUpgradedHttpHandler::Send(std::move(InitRequestData_));
        InitRequestData_.clear();
        RequestUpgraded_ = true;
    } else {
        // error handling
        Closed_ = true;
        TIntrusivePtr<TAsr2Client> self(this);
        OnAnyError(TStringBuilder() << "http_code=" << p.RetCode() << " error=" << error, p.RetCode() == 503);
        NVoicetech::TUpgradedHttpHandler::Cancel();
    }
}

void TAsr2Client::OnRecvMessage(char* data, size_t size) {
    DLOG("client.OnRecvMessage size=" << size);
    NAsr::NProtobuf::TResponse response;
    if (!response.ParseFromArray(data, size)) {
        // TODO: catch parsing exceptions
        OnRecvProtobufError("can not parse protobuf message expect TASRResponse");
        return;
    }
    if (!HasInitResponse_) {
        HasInitResponse_ = true;

        if (!response.HasInitResponse()) {
            OnRecvProtobufError("invalide protobuf message type TASRResponse not contain expected TInitResponse");
            return;
        }

        DLOG("InitResponse: " << response.GetInitResponse().ShortUtf8DebugString());
        if (response.GetInitResponse().GetIsOk()) {
            for (auto& data : PostponedRequests_) {
                DLOG("Send postponed request");
                NVoicetech::TUpgradedHttpHandler::Send(std::move(data));
                if (Closed_) {
                    // on sending error case
                    break;
                }
            }
        }
        PostponedRequests_.clear();
        OnInitResponse(response);
    } else {
        if (response.HasAddDataResponse()) {
            DLOG("AddDataResponse: " << response.GetAddDataResponse().ShortUtf8DebugString());
            OnAddDataResponse(response);
        } else if (!HasCloseConnection_ && response.HasCloseConnection()) {
            TIntrusivePtr<TAsr2Client> self(this);  // keep hereself alive on errors in send
            HasCloseConnection_ = true;
            if (!CloseConnectionSended_) {
                CloseConnectionSended_ = true;
                NAsr::NProtobuf::TRequest request;
                request.MutableCloseConnection();
                Send(request);
            }
            DLOG("AddDataResponse: " << response.GetAddDataResponse().ShortUtf8DebugString());
            OnClose(false);  // false - succesful close
        } else {
            OnRecvProtobufError("invalide protobuf message type TASRResponse not contain expected AddDataResponse or CloseConnection");
            return;
        }
    }
}
