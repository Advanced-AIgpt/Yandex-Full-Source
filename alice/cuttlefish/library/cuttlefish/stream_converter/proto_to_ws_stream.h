#pragma once

#include "asr_recognize.h"
#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/common_items.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/log_context.h>

#include <alice/cuttlefish/library/asr/base/protobuf.h>
#include <alice/cuttlefish/library/music_match/base/protobuf.h>
#include <alice/cuttlefish/library/tts/backend/base/protobuf.h>
#include <alice/cuttlefish/library/yabio/base/protobuf.h>

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/context_save.pb.h>
#include <alice/cuttlefish/library/protos/megamind.pb.h>
#include <alice/cuttlefish/library/protos/bio_context_sync.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/store_audio.pb.h>
#include <alice/cuttlefish/library/protos/uniproxy2.pb.h>

#include <alice/megamind/protos/speechkit/response.pb.h>

#include <voicetech/bio/ondevice/proto_api/bio.pb.h>
#include <voicetech/library/messages/message.h>

#include <util/generic/maybe.h>
#include <util/generic/set.h>

#include <queue>

class TProtobufToWsStreamTest;

namespace NAlice::NCuttlefish::NAppHostServices {
    using TMessage = NVoicetech::NUniproxy2::TMessage;


    class TProtobufToWsStream: public TThrRefBase {
    public:
        TProtobufToWsStream(NAppHost::TServiceContextPtr, TLogContext);

        void OnNextInput();
        bool OnAppHostProtoItem(TStringBuf type, const NAppHost::NService::TProtobufItem&);
        void OnEndInput();
        void OnBreakProcessing();  // when handler error or likely event
        void OnPreFinalMessage();
        void ProcessInputQueue();

        void OnAudio(const NAliceProtocol::TAudio&);
        void OnTtsTimings(const NAlice::NTts::NProtobuf::TTimings&);
        void OnAsrResponse(const NAlice::NAsr::NProtobuf::TResponse&);
        void OnDirective(const NAliceProtocol::TDirective&);
        void OnSessionContext(NAliceProtocol::TSessionContext&&);
        void OnRequestContext(NAliceProtocol::TRequestContext&&);
        void OnContextSaveImportantResponse(const NAliceProtocol::TContextSaveResponse&);
        void OnMusicMatchInitResponse(const NAlice::NMusicMatch::NProtobuf::TInitResponse&);
        void OnMusicMatchStreamResponse(const NAlice::NMusicMatch::NProtobuf::TStreamResponse&);
        void OnYabioResponse(const NAlice::NYabio::NProtobuf::TResponse&);
        void OnBiometryResultForClient(const ::YabioOndeviceProtobuf::BiometryResultForClient& biometryResultForClient);
        void AddVoiceInputBiometryClassification(const NAlice::NYabio::NProtobuf::TAddDataResponse&);
        void OnBioContext(const YabioProtobuf::YabioContext&);
        void OnBioContextSaved();
        void OnMegamindResponse(const NAliceProtocol::TMegamindResponse&);
        void OnStoreAudioResponse(const NAliceProtocol::TStoreAudioResponse&);
        void OnPredefinedAsrResult(const NAliceProtocol::TPredefinedAsrResult&);
        void OnRequestDebugInfo(const NAliceProtocol::TRequestDebugInfo&);
        void OnActivationFinalResponse(const NCachalotProtocol::TActivationFinalResponse&);
        void OnActivationSuccess();
        void OnVoiceprintUpdate(const NAliceProtocol::TEnrollmentUpdateDirective& directive);
        void OnUniproxy2DirectivesSessionLogs(const NAliceProtocol::TUniproxyDirectives& directives);

    private:
        TMaybe<bool> OnAppHostProtoItemImpl(TStringBuf type, const NAppHost::NService::TProtobufItem&);
        void SendSessionLog(const NAliceProtocol::TUniproxyDirective& directive);
        void TryUsePostPonedAsrEou();
        void TryUseSmartActivationLog();
        void AddLegacyVinsTimings();
        void AddLegacyTtsTimings();
        void ForceAddCloseGraph(); // Allowed to use only in one case - closing by timeout. Use TryAddCloseGraph
        void TryAddCloseGraph();
        void AddEventException(const TString& code, const TString& text, const TString& fullText = {});
        void OnNotFatalError(const TString& code, const TString& text);
        void OnEventException(const NAliceProtocol::TEventException&);
        void AddLogAck();
        void AddMessageToAhContext(const TMessage&);
        void AddRequestDebugInfo();
        static bool OutputSpeechContainsSpotterWord(const TString& outputSpeech);
        bool IsVoiceInputOrUndefined();

        NAppHost::TServiceContextPtr AhContext;

    public:
        NThreading::TPromise<void> Promise;

    private:
        friend TProtobufToWsStreamTest;

        class TWatchmen {
        public:
            TWatchmen() {
                RequiredItems_ = {
                    ToString(ITEM_TYPE_STORE_AUDIO_RESPONSE),
                    ToString(ITEM_TYPE_STORE_AUDIO_RESPONSE_SPOTTER),
                    ToString(ITEM_TYPE_UNIPROXY2_DIRECTIVES_SESSION_LOGS)
                };
            }

            void OnNewInputItem(TStringBuf type) {
                auto it = RequiredItems_.find(type);
                if (it != RequiredItems_.end()) {
                    RequiredItems_.erase(it);
                }
            }

            bool CanCloseGraph() {
                WantedCloseGraph_ = true;
                return RequiredItems_.empty();
            }

            bool WantedCloseGraph() {
                return WantedCloseGraph_;
            }

        private:
            TSet<TString> RequiredItems_;
            bool WantedCloseGraph_ = false;
        };

        bool NeedMoreContext() const noexcept {
            return (NeedSessionContext_ && !SessionContext_.Defined())
                || (NeedRequestContext_ && !RequestContext_.Defined());
        }
        void PushMetric(TStringBuf metric, TStringBuf code = "") {
            if (!Metrics_) {
                return;
            }

            Metrics_->PushRate(metric, code);
        }

        TLogContext LogContext;
        THolder<TSourceMetrics> Metrics_;  // WARNING: can be nullptr!

        enum EState {
            ProcessInput,
            Finished
        } State = ProcessInput;
        bool NeedSessionContext_ = true;
        bool NeedRequestContext_ = true;

        // we MUST process SessionContext first, so play other messages to queue for process on right order
        std::queue<std::unique_ptr<google::protobuf::Message>> InputQueue;
        TMaybe<NAliceProtocol::TSessionContext> SessionContext_;
        TMaybe<NAliceProtocol::TRequestContext> RequestContext_;
        TMaybe<NCachalotProtocol::TActivationFinalResponse> SmartActivationResult_;
        TMaybe<NCachalotProtocol::TActivationLog> ActivationLog_;
        TMaybe<NCachalotProtocol::TActivationLog> ActivationLogFinal_;
        THolder<NAliceProtocol::TWsEvent> PostPonedAsrEou_;
        bool AsrEouResultHasEmptyText_ = false;
        bool HasMegamindResponse_ = false;
        bool NeedFlush = false;
        bool OnPreFinalMessageProcessed_ = false;

        TString RefMessageId_;
        // MUST be ui32!
        ui32 RefStreamId_ = 0;
        ui32 InetRefStreamId_ = 0;

        // crutchs for emulate Vins.VoiceInput Biometry.Classification behaviour
        size_t VoiceInputYabioPartialNumber_ = 0;
        // keep here last result for sending right after asr EOU (wokaround for abezhin@ biometry tests)
        TMaybe<NAlice::NYabio::NProtobuf::TAddDataResponse> VoiceInputBiometryClassifyPostponed_;
        bool VoiceInputAsrEouSended_ = false;
        bool CloseGraphSended_ = false;

        TString AsrServerVersion_;
        TString AsrTopic_;
        TString AsrTopicVersion_;
        TString AsrHostname_;
        bool ValidationInvokedUsed_ = false;
        bool SpotterValidationSended_ = false;
        bool HasSmartActivationError_ = false;
        TMaybe<bool> AsrSpotterValid_;  // spotter validation result from ASR
        ui32 AsrPartialNumber_ = 0;  // TODO: remove after update asr_adapters (to version with AddDataResponse::Number)
        NAliceProtocol::TRequestDebugInfo DebugInfo_;
        bool ContainsSpotterWordInOutputSpeech_ = false;
        TWatchmen Watchmen; // he will count input events to understand, when we can close graph
    };
}
