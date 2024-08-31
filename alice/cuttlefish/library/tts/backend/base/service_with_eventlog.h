#pragma once
#include "service.h"
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/proto_censor/tts.h>

namespace NAlice::NTts {
    class TServiceWithEventlog : public TService {
    public:
        explicit TServiceWithEventlog();
        class TRequestProcessor: public TService::TRequestProcessor {
        public:
            explicit TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext);
            TIntrusivePtr<TInterface::TCallbacks> CreateTtsCallbacks() override;

            void OnBackendRequest(const NProtobuf::TBackendRequest& backendRequest, const TStringBuf& itemType) override;
            void OnAppHostEmptyInput() override;
            void OnAppHostClose() override;
            void OnUnknownItemType(const TString& tag, const TString& type) override;
            void OnWarning(const TString& text) override;
            void OnError(const TString& text) override;

        protected:
            NCuttlefish::TLogContext LogContext_;
            TAtomicBase Number_ = 0;
            static TAtomicCounter NextProcNumber_;
        };

    public:
        TIntrusivePtr<TService::TRequestProcessor> CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) override;
    };
}
