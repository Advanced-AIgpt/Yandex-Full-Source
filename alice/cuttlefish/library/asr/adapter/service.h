#pragma once

#include "asr2.h"
#include "asr2_via_asr1_client.h"
#include "asr_callbacks_with_eventlog.h"
#include "protocol_convertor.h"

#include <alice/cuttlefish/library/asr/base/service.h>
#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/proto_configs/asr_adapter.cfgproto.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <voicetech/library/ws_server/http_client.h>

#include <apphost/api/service/cpp/service.h>
#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NAlice::NAsrAdapter {
    class TService : public NAsr::TService {
    public:
        typedef NAliceAsrAdapterConfig::Config TConfig;
        static const TString DefaultConfigResource;

        TService(const TConfig& config);

        const NAliceAsrAdapterConfig::TConfig& Config() noexcept {
            return Config_;
        }

        // expand base asr request processor with log & asr's impl
        class TRequestProcessor: public NAsr::TService::TRequestProcessor {
        public:
            TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext)
                : NAsr::TService::TRequestProcessor(service)
                , Service_(service)
                , LogContext_(std::move(logContext))
                , Number_(NextProcNumber_.Inc())
            {
                LogContext_.LogEventInfoCombo<NEvClass::AppHostProcessor>(Number_);
            }

            void OnBeginProcessInput() override;
            void OnContextLoadResponse(const NAliceProtocol::TContextLoadResponse&);

            bool OnCuttlefishAudio(const NAliceProtocol::TAudio& audio) override {
                TString reqStr;
                if (audio.HasChunk()) {
                    NAliceProtocol::TAudio audioExceptChunk = audio;
                    audioExceptChunk.ClearChunk();
                    if (LogContext_.Options().WriteInfoToEventLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
                        LogContext_.LogEventInfo(NEvClass::RecvFromAppHostAudioChunk(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString()));
                    }
                } else {
                    if (LogContext_.Options().WriteInfoToEventLog || LogContext_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
                        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
                    }
                }
                return NAsr::TService::TRequestProcessor::OnCuttlefishAudio(audio);
            }
            void OnAppHostEmptyInput() override {
                LogContext_.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
                NAsr::TService::TRequestProcessor::OnAppHostEmptyInput();
            }
            void OnAppHostClose() override {
                LogContext_.LogEventInfoCombo<NEvClass::ProcessCloseProcessor>();
                NAsr::TService::TRequestProcessor::OnAppHostClose();
            }
            void OnWarning(const TString& text) override {
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>(text);
            }

            TIntrusivePtr<NAsr::TInterface::TCallbacks> CreateAsrCallbacks(TRequestHandlerPtr, const TString& requestId) override;
            bool OnAppHostProtoItem(const TString& type, const NAppHost::NService::TProtobufItem&) override;
            bool OnInitRequest(NAsr::NProtobuf::TRequest&, TIntrusivePtr<NAsr::TInterface::TCallbacks>, const TString& requestId) override;

        private:
            void OnInitRequestImpl(NAsr::NProtobuf::TRequest&, TIntrusivePtr<NAsr::TInterface::TCallbacks>, const TString& requestId);

            TService& Service_;
            NCuttlefish::TLogContext LogContext_;
            TAtomicBase Number_ = 0;
            static TAtomicCounter NextProcNumber_;

            // Params of init request filled with ContextLoadResponse
            AsrEngineRequestProtobuf::TUserInfo UserInfo_;
            TMaybe<TString> AsrAbFlagsSerializedJson_ = Nothing();
            TMaybe<NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective> PatchAsrOptionsForNextRequestDirective_ = Nothing();
        };

        TIntrusivePtr<NAsr::TService::TRequestProcessor> CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) override {
            return new TRequestProcessor(*this, NAlice::NCuttlefish::LogContextFor(ctx, rtLogClient));
        }
        void StartAsrRequest(NVoicetech::THttpClient&, const NVoicetech::TUpgradedHttpHandlerRef&, const TString& rtLogToken);
        NVoicetech::THttpClient& GetHttpClient() {
            return *HttpClients_[ClientRequestNum_.Inc() % HttpClients_.size()];
        }
        const TString& AsrUrl() const noexcept {
            return AsrUrl_;
        }
        const TAsrInfo& AsrInfo() const noexcept {
            return AsrInfo_;
        }

    private:
        NAliceAsrAdapterConfig::TConfig Config_;
        NAsio::TExecutorsPool ExecutorsPool_;
        NVoicetech::TAsioClients ClientsCount_;  // use for suspend asio executors (wait destroy all asr clients)
        TAtomicCounter ClientRequestNum_;
        TVector<THolder<NVoicetech::THttpClient>> HttpClients_;
        TString AsrUrl_;
        TAsrInfo AsrInfo_;
    };
}
