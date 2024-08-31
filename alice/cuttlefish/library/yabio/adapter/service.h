#pragma once

#include "yabio.h"
#include "yabio_client.h"
#include "yabio_callbacks_with_eventlog.h"

#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/yabio_adapter.cfgproto.pb.h>
#include <alice/cuttlefish/library/yabio/base/service.h>

#include <voicetech/library/ws_server/http_client.h>

#include <apphost/api/service/cpp/service.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NAlice::NYabioAdapter {
    class TService : public NYabio::TService {
    public:
        typedef NAliceYabioAdapterConfig::Config TConfig;
        static const TString DefaultConfigResource;

        TService(const TConfig& config);

        const NAliceYabioAdapterConfig::TConfig& Config() noexcept {
            return Config_;
        }

        // expand base yabio request processor with log & yabio's impl
        class TRequestProcessor: public NYabio::TService::TRequestProcessor {
        public:
            TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext)
                : NYabio::TService::TRequestProcessor(service)
                , Service_(service)
                , Log_(std::move(logContext))
                , Number_(NextProcNumber_.Inc())
                , NeedResultStep_(service.Config().yabio().need_result_step())
            {
                Log_.LogEventInfoCombo<NEvClass::AppHostProcessor>(Number_);
            }
            ~TRequestProcessor() override;
            bool OnAppHostProtoItem(const TString& type, const NAppHost::NService::TProtobufItem&) override;
            bool OnCuttlefishAudio(NAliceProtocol::TAudio&, bool postponed) override;
            void OnIgnoreInitRequest(NYabio::NProtobuf::TInitRequest&, const TString& reason) override;
            bool OnInitRequest(TIntrusivePtr<NYabio::TInterface::TCallbacks>, const TString& requestId) override;
            bool OnYabioContext(bool hasData, const TString& data) override;
            void OnAsrFinished(const NAliceProtocol::TAsrFinished&) override;
            void OnAppHostEmptyInput() override;
            void OnAppHostClose() override;
            void OnWarning(const TString& text) override;
            void OnError(NAlice::NYabio::NProtobuf::EResponseCode, const TString& error) override;
            void SendFastInitResponseError(NAlice::NYabio::NProtobuf::EResponseCode) override;

            TIntrusivePtr<NYabio::TInterface::TCallbacks> CreateYabioCallbacks(const TString& requestId) override;

        private:
            void OnInitRequestImpl(TIntrusivePtr<NYabio::TInterface::TCallbacks>, const TString& requestId);

            TService& Service_;
            NAlice::NCuttlefish::TLogContext Log_;
            TAtomicBase Number_ = 0;
            static TAtomicCounter NextProcNumber_;
            TDuration NeedResultStep_;
        };

        TIntrusivePtr<NYabio::TService::TRequestProcessor> CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) override {
            return new TRequestProcessor(*this, NAlice::NCuttlefish::LogContextFor(ctx, rtLogClient));
        }
        NVoicetech::THttpClient& GetHttpClient() {
            return *HttpClients_[ClientRequestNum_.Inc() % HttpClients_.size()];
        }
        const TString& YabioUrl() noexcept {
            return YabioUrl_;
        }

    private:
        NAliceYabioAdapterConfig::TConfig Config_;
        NAsio::TExecutorsPool ExecutorsPool_;
        NVoicetech::TAsioClients ClientsCount_;  // use for suspend asio executors (wait destroy all yabio clients)
        TAtomicCounter ClientRequestNum_;
        TVector<THolder<NVoicetech::THttpClient>> HttpClients_;
        TString YabioUrl_;
    };
}
