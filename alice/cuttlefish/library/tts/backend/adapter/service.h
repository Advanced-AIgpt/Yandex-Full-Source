#pragma once

#include "tts_client.h"

#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/log_context.h>

#include <alice/cuttlefish/library/proto_censor/tts.h>
#include <alice/cuttlefish/library/proto_configs/tts_adapter.cfgproto.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <alice/cuttlefish/library/tts/backend/base/service_with_eventlog.h>

#include <apphost/api/service/cpp/service.h>

#include <voicetech/library/ws_server/http_client.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NAlice::NTtsAdapter {

    class TService : public NTts::TServiceWithEventlog {
    public:
        using TConfig = NAliceTtsAdapterConfig::Config;
        static const TString DefaultConfigResource;

        explicit TService(const TConfig& config);

        const NAliceTtsAdapterConfig::TConfig& Config() noexcept {
            return Config_;
        }

        TIntrusivePtr<NTts::TService::TRequestProcessor> CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) override {
            return new TRequestProcessor(*this, NAlice::NCuttlefish::LogContextFor(ctx, rtLogClient));
        }

        // Expand base tts request processor with tts's impl
        class TRequestProcessor: public NTts::TServiceWithEventlog::TRequestProcessor {
        public:
            explicit TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext)
                : NTts::TServiceWithEventlog::TRequestProcessor(service, std::move(logContext))
                , Service_(service)
            {
            }

            void InitializeTts(TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks) override;

        private:
            TService& Service_;
        };

        const TTtsClient::TConfig& GetTtsMainClientConfig() const {
            return TtsMainClientConfig_;
        }

        NVoicetech::THttpClient& GetHttpClient() {
            return *HttpClients_[ClientRequestNum_.Inc() % HttpClients_.size()];
        }

    private:
        NAliceTtsAdapterConfig::TConfig Config_;
        TTtsClient::TConfig TtsMainClientConfig_;
        NAsio::TExecutorsPool ExecutorsPool_;
        // Use for suspend asio executors (wait destroy all tts clients)
        NVoicetech::TAsioClients ClientsCount_;
        TAtomicCounter ClientRequestNum_;
        TVector<THolder<NVoicetech::THttpClient>> HttpClients_;
    };
}
