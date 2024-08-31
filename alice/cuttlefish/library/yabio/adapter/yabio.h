#pragma once

#include "yabio_client.h"

#include <alice/cuttlefish/library/yabio/base/interface.h>

#include <alice/cuttlefish/library/logging/log_context.h>


namespace NAlice::NYabioAdapter {
    class TYabio : public NYabio::TInterface {
    public:
        TYabio(NVoicetech::THttpClient&, const TString& url, TIntrusivePtr<TCallbacks>&, const NAlice::NCuttlefish::TLogContext& log, TDuration needResultStep);

        // impl TYabioInterface
        bool ProcessInitRequest(NYabio::NProtobuf::TInitRequest&) override;  // not used method
        bool ProcessAddData(NYabio::NProtobuf::TAddData&) override;
        void CauseError(NYabio::NProtobuf::EResponseCode, const TString& error) override;
        void Close() override;
        void SoftClose() override;

        void SafeInjectYabioResponse(NYabio::NProtobuf::TResponse&&);

        NVoicetech::TUpgradedHttpHandlerRef Handler() {
            return YabioClient_.Get();
        }

    protected:
        class TMyYabioClient : public TYabioClient {
        public:
            TMyYabioClient(const NYabio::NProtobuf::TInitRequest&, TIntrusivePtr<NYabio::TInterface::TCallbacks>&, NRTLog::TRequestLoggerPtr rtLog);
            ~TMyYabioClient() {
                if (YabioCallbacks_) {
                    YabioCallbacks_->Finish();
                }
            }

            // impl. yabio_client callbacks
            void OnSendInitRequest() override;
            void OnSendAddData(size_t) override;
            void OnInitResponse(NYabio::NProtobuf::TResponse&) override;
            void OnAddDataResponse(NYabio::NProtobuf::TResponse&) override;
            void OnClosed() override;
            void OnAnyError(NYabio::NProtobuf::EResponseCode, const TString& error, bool fastError) override;
            void OnYabioResponse(NYabio::NProtobuf::TResponse&);

            TIntrusivePtr<NYabio::TInterface::TCallbacks> YabioCallbacks_;
            NAlice::NCuttlefish::TLogContext CallbacksLog_;  // log for asio thread
            TString GroupId_;
        };

        NVoicetech::THttpClient& HttpClient_;
        NAsio::TIOService& IOService_;
        NAlice::NCuttlefish::TLogContext Log_;
        TIntrusivePtr<TMyYabioClient> YabioClient_;
        TString YabioUrl_;
        TDuration NeedResultStep_;
        TInstant NextNeedResult_;
    };
}
