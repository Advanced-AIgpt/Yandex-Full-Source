#include "yabio_client.h"

#include <util/string/builder.h>

#undef DLOG
#define DLOG(msg)

using namespace NAlice::NYabioAdapter;

namespace {
    size_t CountSize(const TVector<TString>& data) {
        size_t size = 0;
        for (auto& it : data) {
            size += it.size();
        }
        return size;
    }
}

TYabioClient::~TYabioClient() {
    DLOG("~TYabioClient()");
}

void TYabioClient::Send(NAsio::TIOService& ioService, const NYabio::NProtobuf::TAddData& addData) {
    if (Closed_) {
        return;
    }

    TVector<TString> data;
    Serialize(addData, data);
    bool lastChunk = addData.GetlastChunk();
    bool needResult = addData.GetneedResult();
    DLOG("client.--Send AddData");

    ioService.Post([self = TIntrusivePtr(this), data2 = std::move(data), lastChunk, needResult]() mutable {
        DLOG("client.-Send AddData");
        if (self->Closed_ || self->LastChunkSended_) {
            DLOG("client.-Send AddData: closed c=" << int(self->Closed_) << " l=" << int(self->LastChunkSended_));
            return;
        }

        self->ChunksEnqueuedForSending_ += 1;
        if (needResult || lastChunk) {
            self->LastNeedResultChunk_ = self->ChunksEnqueuedForSending_;
        }
        if (lastChunk) {
            DLOG("LastChunkSended !!!!");
            self->LastChunkSended_ = true;
            self->LastNeedResultChunk_ = self->ChunksSended_;
        }

        if (!self->HasInitResponse_) {
            self->PostponedAddData_.emplace_back(std::move(data2));
            return;
        }

        DLOG("client.Send AddData");
        self->ChunksSended_ += 1;
        self->OnSendAddData(CountSize(data2));
        self->NVoicetech::TUpgradedHttpHandler::Send(std::move(data2));
    });
}

void TYabioClient::CauseError(NAsio::TIOService& ioService, NYabio::NProtobuf::EResponseCode responseCode, const TString& error) {
    if (Closed_) {
        return;
    }

    ioService.Post([self = TIntrusivePtr(this), responseCode, error = TString(error)]() mutable {
        if (self->Closed_) {
            return;
        }

        self->Closed_ = true;
        self->OnAnyError(responseCode, error);
        self->ShutdownSending();
    });
}

void TYabioClient::SafeCancel(NAsio::TIOService& ioService) {
    if (Closed_) {
        return;
    }

    ioService.Post([self = TIntrusivePtr(this)]() mutable {
        self->Cancel();
    });
}

void TYabioClient::SoftClose(NAsio::TIOService& ioService) {
    if (Closed_) {
        DLOG("SoftClose() Closed");
        return;
    }

    ioService.Post([self = TIntrusivePtr(this)]() mutable {
        self->SoftCloseImpl();
    });
}

void TYabioClient::SoftCloseImpl() {
    if (Closed_) {
        DLOG("SoftCloseImpl() Closed");
        return;
    }

    DLOG("SoftCloseImpl() pa=" << PostponedAddData_.size() << " chs=" << ChunksSended_ << " chp=" << ChunksProcessed_ << " ln=" << LastNeedResultChunk_);
    if ((PostponedAddData_.size() + ChunksSended_) == ChunksProcessed_
        || ChunksProcessed_ > LastNeedResultChunk_
    ) {
        DLOG("SoftCloseImpl() Shutdown ln=" << LastNeedResultChunk_ << " p=" << ChunksProcessed_ << " s=" << ChunksSended_);
        Closed_ = true;
        // now yabio.alice.yandex.net ignore tcp.FIN for marking end of stream, so use Cancel instead ShutdownSending()
        // ShutdownSending();
        Cancel();
    } else {
        HasSoftClose_ = true;
    }
}

void TYabioClient::OnError(const NVoicetech::TNetworkError& error) {
    DLOG("client.OnError closed=" << int(Closed_));
    if (Closed_) {
        return;
    }

    Closed_ = true;
    bool fastError = error.Operation == NVoicetech::TNetworkError::OpConnect;
    if (error.Operation == 0 && error.Value() == 125) {
        // reading canceled, so it's not error
        OnClose(/*abnormal=*/ false);
        return;
    }
    OnAnyError(
        NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR,
        TStringBuilder() << "network op=" << int(error.Operation) << " errno=" << error.Value() << ": " << error.Text(),
        fastError
    );
}

void TYabioClient::OnError(const NVoicetech::TTypedError& error) {
    DLOG("client.OnTypedError");
    Closed_ = true;
    OnAnyError(NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, error.Text);
    ShutdownSending();
}

void TYabioClient::OnRecvProtobufError(const TString& error) {
    DLOG("client.OnRecvProtobufError: " << error);
    if (Closed_) {
        return;
    }

    TIntrusivePtr<TYabioClient> self(this);
    OnAnyError(NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, error);
    if (!Closed_) {
        Closed_ = true;
        ShutdownSending();
    }
}

void TYabioClient::OnUpgradeResponse(const THttpParser& p, const TString& error) {
    // used only for client connection
    DLOG("client.OnUpgradeResponse: error=" << error);
    if (!error && p.RetCode() == 101) {
        DLOG("client.Send InitRequest");
        OnSendInitRequest();
        NVoicetech::TUpgradedHttpHandler::Send(std::move(InitRequestData_));
        InitRequestData_.clear();
        RequestUpgraded_ = true;
    } else {
        // error handling
        Closed_ = true;
        TIntrusivePtr<TYabioClient> self(this);
        OnAnyError(NYabio::NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, TStringBuilder() << "http_code=" << p.RetCode() << " error=" << error, p.RetCode() == 503);
        NVoicetech::TUpgradedHttpHandler::Cancel();
    }
}

void TYabioClient::OnRecvMessage(char* data, size_t size) {
    if (!size) {
        OnClose(true);
        return;
    }

    DLOG("client.OnRecvMessage size=" << size);
    if (!HasInitResponse_) {
        NYabio::NProtobuf::TInitResponse initResponse;
        try {
            if (!initResponse.ParseFromArray(data, size)) {
                // TODO: catch parsing exceptions
                OnRecvProtobufError("can not parse protobuf message expect: YabioProtobuf::YabioResponse");
                return;
            }
        } catch (...) {
            OnRecvProtobufError(TStringBuilder() << "fail parse YabioProtobuf::YabioResponse: " << CurrentExceptionMessage());
            return;
        }
        HasInitResponse_ = true;

        if (initResponse.GetresponseCode() == NYabio::NProtobuf::RESPONSE_CODE_OK) {
            for (auto& data : PostponedAddData_) {
                DLOG("client" << ".Send postponed AddData");
                ChunksSended_ += 1;
                OnSendAddData(CountSize(data));
                NVoicetech::TUpgradedHttpHandler::Send(std::move(data));
                if (Closed_) {
                    // on sending error case
                    PostponedAddData_.clear();
                    return; // error already MUST BE handled in OnError() callback
                }
            }
        }
        PostponedAddData_.clear();

        DLOG("InitResponse: " << initResponse.ShortUtf8DebugString());
        NYabio::NProtobuf::TResponse response;
        response.MutableInitResponse()->Swap(&initResponse);
        OnInitResponse(response);
    } else {
        NYabio::NProtobuf::TAddDataResponse addDataResponse;
        try {
            if (!addDataResponse.ParseFromArray(data, size)) {
                // TODO: catch parsing exceptions
                OnRecvProtobufError("can not parse protobuf message expect: YabioProtobuf::AddDataResponse");
                return;
            }
        } catch (...) {
            OnRecvProtobufError(TStringBuilder() << "fail parse YabioProtobuf::AddDataResponse: " << CurrentExceptionMessage());
            return;
        }
        if (addDataResponse.has_context()) {
            auto ts = TInstant::Now().Seconds();
            for (auto& e : (*addDataResponse.mutable_context()->mutable_enrolling())) {
                e.set_request_id(RequestId_);
                e.set_timestamp(ts);
                //TODO: set e.device_model e.device_id e.device_manufacturer
            }
        }
        addDataResponse.set_request_id(RequestId_);

        DLOG("AddDataResponse: " << addDataResponse.ShortUtf8DebugString());
        if (addDataResponse.HasmessagesCount()) {
            ChunksProcessed_ += addDataResponse.GetmessagesCount();
        }
        DLOG("Sended=" << ChunksSended_ << " processed=" << ChunksProcessed_);
        NYabio::NProtobuf::TResponse response;
        response.MutableAddDataResponse()->Swap(&addDataResponse);
        response.SetForMethod(Method_);
        OnAddDataResponse(response);
        if ((LastChunkSended_ && ChunksSended_ == ChunksProcessed_)
            || (HasSoftClose_ && ChunksProcessed_ > LastNeedResultChunk_)
        ) {
            Closed_ = true;
            Cancel();
            OnClose(false);
        }
    }
}
