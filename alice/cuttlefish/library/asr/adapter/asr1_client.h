#pragma once

#include "unistat.h"

#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/event_log.h>

#include <voicetech/library/proto_api/yaldi.pb.h>
#include <voicetech/library/protobuf_handler/protobuf_handler.h>
#include <voicetech/library/ws_server/http_client.h>

#include <library/cpp/neh/asio/executor.h>
#include <library/cpp/threading/atomic/bool.h>

#include <util/generic/ptr.h>

namespace NAlice::NAsrAdapter {
    class TAsr1Client: public NVoicetech::TProtobufHandler {
    public:
        TAsr1Client(const YaldiProtobuf::InitRequest& initRequest, size_t clientNum)
            : ClientNum_(clientNum)
            , UnistatCounterGuard_(Unistat().Asr1ClientCounter())
        {
            Unistat().OnCreateAsr1Client();
            Serialize(initRequest, InitRequestData_);
        }
        ~TAsr1Client() override;

        void SafeSend(NAsio::TIOService&, const YaldiProtobuf::AddData& addData);
        void SafeCauseError(NAsio::TIOService&, const TString& error);
        virtual void SafeClose(NAsio::TIOService&);

        void OnRecv(char* data, size_t size) override {
            Unistat().OnReceiveFromAsrRaw(size);
            NVoicetech::TProtobufHandler::OnRecv(data, size);
        }
        // method can be used only from this request asio thread
        void UnsafeSendCloseConnection(NAsio::TIOService&);

        virtual void UnsafeSend(TVector<TString>&&, bool lastChunk);
        virtual void OnSend() {}
        virtual void OnInitResponse(const YaldiProtobuf::InitResponse&) = 0;
        virtual void OnAddDataResponse(const YaldiProtobuf::AddDataResponse&) = 0;
        virtual void OnClosed() = 0;
        // NOTE: for handling specific errors use overriding OnError|OnRecvProtobufError|OnUpgradeResponse
        // fast error mean retryable (on another instance) error - icmp connection refuse or http_code=503
        virtual void OnAnyError(const TString& error, bool fastError=false, int errorCore=0) = 0;

        void OnClose(bool abnormal) override;
        void OnError(const NVoicetech::TNetworkError& error) override;
        void OnError(const NVoicetech::TTypedError& error) override;
        void OnRecvProtobufError(const TString& error) override;

        void SetIgnoreProtobufParsingError(bool val) {
            IgnoreProtobufParsingError_ = val;
        }
        ui32 ClientNum() const noexcept {
            return ClientNum_;
        }

    protected:
        void SetClosingDeadline(NAsio::TIOService&, TDuration dur);  // can throw exception on closing race
        void ResetClosingDeadline();
        void OnClosingDeadline();

        void OnUpgradeResponse(const THttpParser&, const TString& error) override;
        void OnRecvMessage(char* data, size_t size) override;

        bool IgnoreProtobufParsingError_ = false;
        const ui32 ClientNum_;
        TVector<TString> InitRequestData_;
        struct TPreparedAddData {
            TPreparedAddData(TVector<TString>&& data, bool lastChunk)
                : Data(std::move(data))
                , LastChunk(lastChunk)
            {}
            TPreparedAddData(TPreparedAddData&&) = default;
            TVector<TString> Data;
            bool LastChunk;
        };
        TVector<TPreparedAddData> PostponedAddData_;
        bool RequestUpgraded_ = false;
        bool HasInitResponse_ = false;
        bool LastChunkSended_ = false;
        NAtomic::TBool Closed_ = false;
        TUnistatCounterGuard UnistatCounterGuard_;  // count clients (for detect leaks)
        std::unique_ptr<NAsio::TDeadlineTimer> ClosingDeadlineTimer_;
    };

}
