#include "asr1_client.h"

#include <util/string/builder.h>

#undef DLOG
#define DLOG(msg)

using namespace NAlice::NAsrAdapter;

namespace {
    void CountSendedBytes(const TVector<TString>& data) {
        size_t sz = 0;
        for (const auto& chunk : data) {
            sz += chunk.Size();
        }
        Unistat().OnSendToAsrRaw(sz);
    }
}

TAsr1Client::~TAsr1Client() {
    DLOG("~TAsr1Client:" << ClientNum_);
}

void TAsr1Client::SafeSend(NAsio::TIOService& ioService, const YaldiProtobuf::AddData& addData) {
    if (Closed_) {
        return;
    }

    bool lastChunk = addData.GetlastChunk();
    TVector<TString> data;
    Serialize(addData, data);
    ioService.Post([self = TIntrusivePtr(this), data2 = std::move(data), lastChunk]() mutable {
        if (self->Closed_) {
            return;
        }

        if (!self->HasInitResponse_) {
            self->PostponedAddData_.emplace_back(TPreparedAddData{std::move(data2), lastChunk});
            return;
        }

        DLOG("client" << self->ClientNum_ << ".Send AddData");
        self->UnsafeSend(std::move(data2), lastChunk);
    });
}

void TAsr1Client::SafeCauseError(NAsio::TIOService& ioService, const TString& error) {
    if (Closed_) {
        return;
    }

    ioService.Post([self = TIntrusivePtr(this), error = TString(error)]() mutable {
        if (self->Closed_) {
            return;
        }

        DLOG("client" << self->ClientNum_ << ".CauseError");
        self->OnAnyError(error);
    });
}

void TAsr1Client::SafeClose(NAsio::TIOService& ioService) {
    if (Closed_) {
        return;
    }

    ioService.Post([self = TIntrusivePtr(this), &ioService]() mutable {
        if (self->Closed_) {
            return;
        }

        if (self->RequestUpgraded_ && self->HasInitResponse_) {
            self->UnsafeSendCloseConnection(ioService);
        } else {
            self->Closed_ = true;
            DLOG("client" << self->ClientNum_ << ".close(cancel)");
            self->NVoicetech::TUpgradedHttpHandler::Cancel();
        }
        self->OnClosed();
    });
}

void TAsr1Client::UnsafeSendCloseConnection(NAsio::TIOService& ioService) {
    if (Closed_) {
        return;
    }

    YaldiProtobuf::AddData addData;
    addData.SetlastChunk(true);
    addData.SetcloseConnection(true);
    TVector<TString> data;
    Serialize(addData, data);
    try {
        CountSendedBytes(data);
        NVoicetech::TUpgradedHttpHandler::Send(std::move(data));
        if (!Closed_) {
            SetClosingDeadline(ioService, TDuration::Seconds(2));
        }
    } catch (...) {
        // ignore closing socket (from another side) race errors
    }
    Closed_ = true;
}

void TAsr1Client::SetClosingDeadline(NAsio::TIOService& ioService, TDuration dur) {
    ClosingDeadlineTimer_.reset(new NAsio::TDeadlineTimer(ioService));
    ClosingDeadlineTimer_->AsyncWaitExpireAt(
        dur,
        [self = TIntrusivePtr<TAsr1Client>(this)](const NAsio::TErrorCode& ec, NAsio::IHandlingContext&) {
            if (ec) {
                return;  // ignore canceling
            }

            self->OnClosingDeadline();
        }
    );
}

void TAsr1Client::ResetClosingDeadline() {
    if (ClosingDeadlineTimer_) {
        ClosingDeadlineTimer_->Cancel();
        ClosingDeadlineTimer_.reset();
    }
}

void TAsr1Client::OnClosingDeadline() {
    Cancel();
    if (ClosingDeadlineTimer_) {
        ClosingDeadlineTimer_.reset();
    }
}

void TAsr1Client::UnsafeSend(TVector<TString>&& data, bool lastChunk) {
    if (Closed_ || LastChunkSended_) {
        return;
    }

    OnSend();
    if (lastChunk) {
        LastChunkSended_ = true;
    }
    CountSendedBytes(data);
    NVoicetech::TUpgradedHttpHandler::Send(std::move(data));
}

void TAsr1Client::OnClose(bool abnormal) {
    DLOG("client" << ClientNum_ << ".OnClose(" << abnormal << ")");
    ResetClosingDeadline();
    if (abnormal) {
        OnAnyError(TString("abnormal close"));
    } else {
        OnClosed();
    }
}

void TAsr1Client::OnError(const NVoicetech::TNetworkError& error) {
    ResetClosingDeadline();
    if (Closed_) {
        return;
    }

    DLOG("client" << ClientNum_ << ".OnError(network)");
    Closed_ = true;
    bool fastError = false;
    int errorCode = 0;
    if (error.Operation == NVoicetech::TNetworkError::OpConnect) {
        errorCode = TUnistatN::ConnectFailed;
        fastError = true;
    }
    OnAnyError(TStringBuilder() << "network op=" << int(error.Operation) << " errno=" << error.Value() << ": " << error.Text(), fastError, errorCode);
}

void TAsr1Client::OnError(const NVoicetech::TTypedError& error) {
    ResetClosingDeadline();
    if (Closed_) {
        return;
    }

    DLOG("client" << ClientNum_ << ".OnError(typed)");
    Closed_ = true;
    bool fastError = false;
    int errorCode = 0;
    if (error.Type == NVoicetech::TTypedError::TypeResponseParsing) {
        errorCode = TUnistatN::ParseHttpResponseFailed;
        fastError = true;  // obviously catch such error on shutdown: (THttpException) incompleted http response
    }
    OnAnyError(error.Text, fastError, errorCode);
    NVoicetech::TUpgradedHttpHandler::Cancel();
}

void TAsr1Client::OnUpgradeResponse(const THttpParser& p, const TString& error) {
    // used only for client connection
    DLOG("client" << ClientNum_ << ".OnUpgradeResponse: error=" << error << " code=" << p.RetCode());
    if (Closed_) {
        return;
    }

    if (!error && p.RetCode() == 101) {
        DLOG("client" << ClientNum_ << ".Send InitRequest");
        CountSendedBytes(InitRequestData_);
        NVoicetech::TUpgradedHttpHandler::Send(std::move(InitRequestData_));
        InitRequestData_.clear();
        RequestUpgraded_ = true;
    } else {
        // error handling
        Closed_ = true;
        TIntrusivePtr<TAsr1Client> self(this);
        OnAnyError(TStringBuilder() << "http_code=" << p.RetCode() << " error=" << error, p.RetCode() == 503);
        NVoicetech::TUpgradedHttpHandler::Cancel();
    }
}

void TAsr1Client::OnRecvProtobufError(const TString& error) {
    DLOG("client" << ClientNum_ << ".OnRecvProtobufError: " << error);
    ResetClosingDeadline();
    if (Closed_) {
        return;
    }

    Closed_ = true;
    OnAnyError(error);
    NVoicetech::TUpgradedHttpHandler::Cancel();
}

void TAsr1Client::OnRecvMessage(char* data, size_t size) {
    DLOG("client" << ClientNum_ << ".OnRecvMessage size=" << size);
    if (Closed_) {
        return;
    }

    if (!HasInitResponse_) {
        HasInitResponse_ = true;
        YaldiProtobuf::InitResponse initResponse;
        if (!initResponse.ParseFromArray(data, size) && !IgnoreProtobufParsingError_) {
            OnRecvProtobufError("can not parse protobuf message InitResponse");
            return;
        }

        DLOG("InitResponse" << ClientNum_ << ": " << initResponse.ShortUtf8DebugString());
        if (initResponse.GetresponseCode() == 200) {
            for (auto& data : PostponedAddData_) {
                DLOG("client" << ClientNum_ << ".Send postponed AddData");
                UnsafeSend(std::move(data.Data), data.LastChunk);
                if (Closed_) {
                    // on sending error case
                    break;
                }
            }
        }
        PostponedAddData_.clear();
        OnInitResponse(initResponse);
    } else {
        YaldiProtobuf::AddDataResponse addDataResponse;
        if (!addDataResponse.ParseFromArray(data, size) && !IgnoreProtobufParsingError_) {
            OnRecvProtobufError("can not parse protobuf message AddDataResponse");
            return;
        }

        DLOG("AddDataResponse" << ClientNum_ << ": " << addDataResponse.ShortUtf8DebugString());
        OnAddDataResponse(addDataResponse);
    }
}
