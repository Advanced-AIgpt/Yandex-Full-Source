#pragma once

#include "music_match.h"
#include "music_match_callbacks_with_eventlog.h"

#include <alice/cuttlefish/library/music_match/base/service.h>
#include <alice/cuttlefish/library/logging/apphost_log.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/proto_configs/music_match_adapter.cfgproto.pb.h>

#include <voicetech/library/ws_server/http_client.h>

#include <apphost/api/service/cpp/service.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NAlice::NMusicMatchAdapter {
    class TService : public NMusicMatch::TService {
    public:
        typedef NAliceMusicMatchAdapterConfig::Config TConfig;
        static const TString DefaultConfigResource;

        TService(const TConfig& config);

        const NAliceMusicMatchAdapterConfig::TConfig& Config() noexcept {
            return Config_;
        }

        // expand base music_match request processor with log & music_match's impl
        class TRequestProcessor: public NMusicMatch::TService::TRequestProcessor {
        public:
            TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext)
                : NMusicMatch::TService::TRequestProcessor(service)
                , Service_(service)
                , Log_(std::move(logContext))
                , Number_(NextProcNumber_.Inc())
            {
                Log_.LogEventInfoCombo<NEvClass::AppHostProcessor>(Number_);
            }

            bool OnContextLoadResponse(const NMusicMatch::NProtobuf::TContextLoadResponse& contextLoadResponse) override {
                Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostContextLoadResponse>(contextLoadResponse.ShortUtf8DebugString());
                return NMusicMatch::TService::TRequestProcessor::OnContextLoadResponse(contextLoadResponse);
            }
            bool OnSessionContext(const NMusicMatch::NProtobuf::TSessionContext& sessionContext) override {
                Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostSessionContext>(sessionContext.ShortUtf8DebugString());
                return NMusicMatch::TService::TRequestProcessor::OnSessionContext(sessionContext);
            }
            bool OnTvmServiceTicket(const TString& tvmServiceTicket) override {
                Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostTvmServiceTicket>(tvmServiceTicket);
                return NMusicMatch::TService::TRequestProcessor::OnTvmServiceTicket(tvmServiceTicket);
            }
            bool OnCuttlefishAudio(const NAliceProtocol::TAudio& audio) override {
                if (audio.HasChunk()) {
                    NAliceProtocol::TAudio audioExceptChunk = audio;
                    audioExceptChunk.ClearChunk();
                    Log_.LogEventInfo(NEvClass::RecvFromAppHostAudioChunk(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString()));
                } else {
                    Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
                }
                return NMusicMatch::TService::TRequestProcessor::OnCuttlefishAudio(audio);
            }
            bool OnCuttlefishSpotterAudioChunk(const NAliceProtocol::TAudioChunk& audioChunk) override {
                Log_.LogEventInfo(NEvClass::InfoMessage(TStringBuilder() << "Skip spotter audio chunk: size = " << audioChunk.GetData().size()));
                return NMusicMatch::TService::TRequestProcessor::OnCuttlefishSpotterAudioChunk(audioChunk);
            }
            bool OnStreamRequest(const NMusicMatch::NProtobuf::TStreamRequest& streamRequest) override {
                if (streamRequest.HasAddData()) {
                    const auto& addData = streamRequest.GetAddData();
                    Log_.LogEventInfo(
                        NEvClass::RecvFromAppHostMusicMatchStreamRequest(
                            TStringBuilder() << TStringBuf("AddData AudioData.size=") << addData.GetAudioData().size()
                        )
                    );
                } else {
                    Log_.LogEventInfoCombo<NEvClass::RecvFromAppHostMusicMatchStreamRequest>(streamRequest.ShortUtf8DebugString());
                }
                return NMusicMatch::TService::TRequestProcessor::OnStreamRequest(streamRequest);
            }

            bool OnInitRequest(const NMusicMatch::NProtobuf::TInitRequest& initRequest) override;

            void InitializeMusicMatch(TIntrusivePtr<NMusicMatch::TInterface::TCallbacks> callbacks) override;
            TIntrusivePtr<NMusicMatch::TInterface::TCallbacks> CreateMusicMatchCallbacks(NAppHost::TServiceContextPtr& ctx) override;

            void OnAppHostEmptyInput() override {
                Log_.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
                NMusicMatch::TService::TRequestProcessor::OnAppHostEmptyInput();
            }
            void OnAppHostClose(NThreading::TPromise<void>& promise, bool requestCancelled) override {
                Log_.LogEventInfoCombo<NEvClass::ProcessCloseProcessor>();
                NMusicMatch::TService::TRequestProcessor::OnAppHostClose(promise, requestCancelled);
            }
            void OnUnknownItemType(const TString& tag, const TString& type) override {
                Log_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << TStringBuf("unknown item tag=") << tag << TStringBuf(", type=") << type);
                NMusicMatch::TService::TRequestProcessor::OnUnknownItemType(tag, type);
            }
            void OnWarning(const TString& warning) override {
                Log_.LogEventErrorCombo<NEvClass::WarningMessage>(warning);
                NMusicMatch::TService::TRequestProcessor::OnWarning(warning);
            }
            void OnUnknownCuttlefishAudioMessageType(const NAliceProtocol::TAudio& audio) override {
                TString audioMessageTypeName;
                TString audioMetaInfoTypeName;
                {
                    auto audioMessageCase = audio.GetMessageCase();
                    if (audioMessageCase == NAliceProtocol::TAudio::MessageCase::MESSAGE_NOT_SET) {
                        audioMessageTypeName = "Audio message type not set";
                    } else {
                        auto* fieldDescriptor =  NAliceProtocol::TAudio::descriptor()->FindFieldByNumber(audioMessageCase);
                        audioMessageTypeName = fieldDescriptor ? fieldDescriptor->name() : "Totally unknown audio message type";
                    }
                }
                {
                    auto audioMetaInfoCase = audio.GetMetaInfoCase();
                    if (audioMetaInfoCase == NAliceProtocol::TAudio::MetaInfoCase::METAINFO_NOT_SET) {
                        audioMessageTypeName = "Audio meta info not set";
                    } else {
                        auto* fieldDescriptor =  NAliceProtocol::TAudio::descriptor()->FindFieldByNumber(audioMetaInfoCase);
                        audioMetaInfoTypeName = fieldDescriptor ? fieldDescriptor->name() : "Totally unknown audio meta info type";
                    }
                }
                Log_.LogEventErrorCombo<NEvClass::WarningMessage>(
                    TStringBuilder()
                        << "Unknown cuttlefish audio message: type=" << audioMessageTypeName
                        << ", meto_info_type=" << audioMetaInfoTypeName
                );
                NMusicMatch::TService::TRequestProcessor::OnUnknownCuttlefishAudioMessageType(audio);
            }

            void OnError(const TString& error) override {
                Log_.LogEventErrorCombo<NEvClass::ErrorMessage>(error);
                NMusicMatch::TService::TRequestProcessor::OnError(error);
            }

        private:
            TService& Service_;
            NCuttlefish::TLogContext Log_;
            TAtomicBase Number_ = 0;
            static TAtomicCounter NextProcNumber_;
        };

        void StartMusicMatchRequest(NVoicetech::THttpClient&, const NVoicetech::TWsHandlerRef&, const TString& headers);

        TIntrusivePtr<NMusicMatch::TService::TRequestProcessor> CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) override {
            return new TRequestProcessor(*this, NAlice::NCuttlefish::LogContextFor(ctx, rtLogClient));
        }
        NVoicetech::THttpClient& GetHttpClient() {
            return *HttpClients_[ClientRequestNum_.Inc() % HttpClients_.size()];
        }
        const TString& MusicMatchUrl() noexcept {
            return MusicMatchUrl_;
        }

    private:
        NAliceMusicMatchAdapterConfig::TConfig Config_;
        NAsio::TExecutorsPool ExecutorsPool_;
        NVoicetech::TAsioClients ClientsCount_;  // use for suspend asio executors (wait destroy all music_match clients)
        TAtomicCounter ClientRequestNum_;
        TVector<THolder<NVoicetech::THttpClient>> HttpClients_;
        TString MusicMatchUrl_;
    };
}
