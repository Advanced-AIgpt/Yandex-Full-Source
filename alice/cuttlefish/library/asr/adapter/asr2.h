#pragma once

#include "asr2_client.h"
#include "protocol_convertor.h"

#include <alice/cuttlefish/library/asr/base/interface.h>
#include <alice/cuttlefish/library/logging/log_context.h>

namespace NAlice::NAsrAdapter {
    class TAsr2 : public NAsr::TInterface {
    public:
        TAsr2(
            const NAsr::NProtobuf::TRequest& initRequest,
            TIntrusivePtr<TCallbacks>& callbacks,
            const NCuttlefish::TLogContext& logContextForCallbacks,
            const NCuttlefish::TLogContext& logContext
        );

        // impl TAsrInterface
        bool ProcessAsrRequest(const NAsr::NProtobuf::TRequest& asrRequest) override;
        void CauseError(const TString& error) override;
        void Close() override;

        void SafeInjectAsrResponse(NAsr::NProtobuf::TResponse&&);

        NVoicetech::TUpgradedHttpHandlerRef Handler() {
            return Asr2Client_.Get();
        }

    protected:
        class TMyAsr2Client : public TProtocolConvertor, public TAsr2Client {
        public:
            TMyAsr2Client(const NAsr::NProtobuf::TRequest&, TIntrusivePtr<NAsr::TInterface::TCallbacks>&);

            // impl. asr2_client callbacks
            void OnInitResponse(NAsr::NProtobuf::TResponse&) override;
            void OnAddDataResponse(NAsr::NProtobuf::TResponse&) override;
            void OnClosed() override;
            void OnAnyError(const TString& error, bool fastError) override;
            void OnAsrResponse(NAsr::NProtobuf::TResponse&);

            TIntrusivePtr<NAsr::TInterface::TCallbacks> Asr2Callbacks_;
        };

        TIntrusivePtr<TMyAsr2Client> Asr2Client_;
        NCuttlefish::TLogContext CallbacksLog_;
        NCuttlefish::TLogContext Log_;
    };
}
