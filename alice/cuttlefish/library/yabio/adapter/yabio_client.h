#pragma once

#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/yabio/base/interface.h>

#include <voicetech/library/protobuf_handler/protobuf_handler.h>
#include <voicetech/library/ws_server/http_client.h>

#include <library/cpp/neh/asio/executor.h>
#include <library/cpp/threading/atomic/bool.h>

#include <util/generic/ptr.h>

namespace NAlice::NYabioAdapter {
    class TYabioClient: public NVoicetech::TProtobufHandler {
    public:
        TYabioClient(const NYabio::NProtobuf::TInitRequest& initRequest) {
            Method_ = initRequest.method();
            RequestId_ = initRequest.GetMessageId();
            if (initRequest.HasTestRequestId()) {
                RequestId_ = initRequest.GetTestRequestId();
            }
            Serialize(initRequest, InitRequestData_);
            InitRequestDebugString_ = initRequest.ShortUtf8DebugString();
        }
        ~TYabioClient();

        // thread-safe methods
        void Send(NAsio::TIOService&, const NYabio::NProtobuf::TAddData&);
        void CauseError(NAsio::TIOService&, NYabio::NProtobuf::EResponseCode, const TString& error);
        void SafeCancel(NAsio::TIOService&);
        void SoftClose(NAsio::TIOService&);

    protected:
        void SoftCloseImpl();

        virtual void OnSendInitRequest() {}
        virtual void OnSendAddData(size_t) {}
        virtual void OnInitResponse(NYabio::NProtobuf::TResponse&) = 0;
        virtual void OnAddDataResponse(NYabio::NProtobuf::TResponse&) = 0;
        virtual void OnClosed() = 0;
        virtual void OnAnyError(NYabio::NProtobuf::EResponseCode, const TString& error, bool fastError=false) = 0;

        void OnClose(bool abnormal) override {
            (void)abnormal;
            if (!HasOnCloseCall_) {
                HasOnCloseCall_ = true;
                OnClosed();
            }
            // TODO:
        }
        void OnError(const NVoicetech::TNetworkError& error) override;
        void OnError(const NVoicetech::TTypedError& error) override;
        void OnRecvProtobufError(const TString& error) override;

    protected:
        // TVoicetech::TUpgradedHttpHandler api impl
        void OnUpgradeResponse(const THttpParser&, const TString& error) override;
        void OnRecvMessage(char* data, size_t size) override;

        NYabio::NProtobuf::EMethod Method_;
        TString RequestId_;
        TVector<TString> InitRequestData_;
        TString InitRequestDebugString_;
        TVector<TVector<TString>> PostponedAddData_;
        bool RequestUpgraded_ = false;
        bool HasInitResponse_ = false;
        size_t ChunksEnqueuedForSending_ = 0;
        size_t LastNeedResultChunk_ = 0;
        size_t ChunksSended_ = 0;
        size_t ChunksProcessed_ = 0;
        bool LastChunkSended_ = false;
        bool HasSoftClose_ = false;
        //bool HasCloseCnnection_ = false;
        bool HasOnCloseCall_ = false;
        //bool CloseConnectionSended_ = false;
        NAtomic::TBool Closed_ = false;
    };

}
