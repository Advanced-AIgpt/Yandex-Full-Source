#pragma once

#include "cloud_synth.h"

#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/log_context.h>

#include <alice/cuttlefish/library/proto_censor/tts.h>
#include <alice/cuttlefish/library/proto_configs/cloud_synth.cfgproto.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <alice/cuttlefish/library/tts/backend/base/service_with_eventlog.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCloudSynth {
    class TService : public NTts::TServiceWithEventlog {
    public:
        using TConfig = NAliceCloudSynthConfig::Config;
        static const TString DefaultConfigResource;

        explicit TService(const TConfig& config);

        const TConfig& Config() noexcept {
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

        const TCloudSynth::TConfig& GetCloudSynthConfig() const {
            return CloudSynthConfig_;
        }

    private:
        TConfig Config_;
        TCloudSynth::TConfig CloudSynthConfig_;
    };
}
