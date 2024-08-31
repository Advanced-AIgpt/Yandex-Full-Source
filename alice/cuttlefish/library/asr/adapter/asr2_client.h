#pragma once

#include <alice/cuttlefish/library/asr/base/interface.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/protobuf_handler/protobuf_handler.h>
#include <voicetech/library/ws_server/http_client.h>

#include <library/cpp/neh/asio/executor.h>
#include <library/cpp/threading/atomic/bool.h>

#include <util/generic/ptr.h>

namespace NAlice::NAsrAdapter {
    class TAsr2Client: public NVoicetech::TProtobufHandler {
    public:
        TAsr2Client(const NAsr::NProtobuf::TRequest& asrInitRequest) {
            if (!asrInitRequest.HasInitRequest()) {
                throw yexception() << "TAsr2Client MUST be constructed with InitRequest";
            }

            Serialize(asrInitRequest, InitRequestData_);
        }

        void Send(const NAsr::NProtobuf::TRequest&);
        void CauseError(const TString& error);
        void SafeClose();

        virtual void OnInitResponse(NAsr::NProtobuf::TResponse&) = 0;
        virtual void OnAddDataResponse(NAsr::NProtobuf::TResponse&) = 0;
        virtual void OnClosed() = 0;
        virtual void OnAnyError(const TString& error, bool fastError=false) = 0;

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
        void OnUpgradeResponse(const THttpParser&, const TString& error) override;
        void OnRecvMessage(char* data, size_t size) override;

        TVector<TString> InitRequestData_;
        TVector<TVector<TString>> PostponedRequests_;
        bool RequestUpgraded_ = false;
        bool HasInitResponse_ = false;
        bool HasCloseConnection_ = false;
        bool HasOnCloseCall_ = false;
        bool CloseConnectionSended_ = false;
        NAtomic::TBool Closed_ = false;
    };

}
