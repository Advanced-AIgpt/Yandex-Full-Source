#pragma once

#include "service.h"

#include "asr_recognize.h"
#include "music_match_request.h"
#include "support_functions.h"

#include <alice/cuttlefish/library/cuttlefish/stream_converter/matched_user_event_handler.h>
#include <alice/cuttlefish/library/cuttlefish/common/common_items.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <alice/cuttlefish/library/experiments/experiments.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/messages/message.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    using TMessage = NVoicetech::NUniproxy2::TMessage;

    class TWsStreamToProtobuf: public TThrRefBase {
    private:
        struct TAudioFlags {
           enum : unsigned {
                ASR                = 1,
                MUSIC_MATCH        = 1 << 1,
                YABIO_CLASSIFY     = 1 << 2,
                YABIO_SCORE        = 1 << 3,
            };

            unsigned Bits;

            TAudioFlags(unsigned bits)
                : Bits(bits)
            {}

            inline bool HasAsr() {
                return Bits & ASR;
            }
            inline bool HasYabioClassify() {
                return Bits & YABIO_CLASSIFY;
            }
            inline bool HasYabioScore() {
                return Bits & YABIO_SCORE;
            }
            inline bool HasMusicMatch() {
                return Bits & MUSIC_MATCH;
            }
            inline unsigned GetBits() {
                return Bits;
            }
            inline void Reset(unsigned bits) noexcept {
                Bits &= ~bits;
            }

            inline TAudioFlags operator|(unsigned bits) const noexcept {
                return Bits | bits;
            }
            inline TAudioFlags operator&(unsigned bits) const noexcept {
                return Bits & bits;
            }
            inline TAudioFlags& operator|=(unsigned bits) noexcept {
                Bits |= bits;
                return *this;
            }
            inline TAudioFlags& operator&=(unsigned bits) noexcept {
                Bits &= bits;
                return *this;
            }
        };

    public:
        TWsStreamToProtobuf(NAppHost::TServiceContextPtr, const NVoice::NExperiments::TExperiments&, TLogContext);

        void OnNextInput();

    private:
        bool OnAppHostProtoItem(TStringBuf type, const NAppHost::NService::TProtobufItem& item);
        void OnEndInput();
        void OnBreakProcessing();  // when handler error or likely event

        void OnAsrRecognize(const TMessage&);
        bool OnTtsGenerate(const TMessage&);
        void OnVinsRequest(const TMessage&);
        void OnVinsVoiceInput(const TMessage&);
        void OnVinsTextInput(const TMessage&);
        void OnVinsMusicInput(const TMessage&);
        void OnLogSpotter(const TMessage&);
        void OnBiometryClassify(const TMessage&);
        void OnBiometryIdentify(const TMessage&);
        void OnBiometryCreateOrUpdateUser(const TMessage&);
        void OnBiometryGetUsers(const TMessage&);
        void OnBiometryRemoveUser(const TMessage&);
        void OnUserMatchedResult(const TMessage&);
        void OnStreamControl(const NAliceProtocol::TEventHeader&);

        void ProcessAsrInitData(const NAlice::NAsr::NProtobuf::TInitRequest&);
        void ProcessMusicMatchInitData(const NAlice::NMusicMatch::NProtobuf::TInitRequest&);
        void ProcessYabioInitClassifyData(const YabioProtobuf::YabioRequest&);
        void ProcessYabioInitScoreData(const YabioProtobuf::YabioRequest&);

        void StartAudio(const TMessage& message, TAudioFlags audioFlags);
        void SendStartStream(const TString& mime, bool hasSpotter=false);
        void SendStartSpotter();

        void StartContextLoad(const TMessage&);
        void StartGuestContextLoad(const TMessage&);
        void StartLoadBiometryContext(const TMessage&);
        void StartCreateOrUpdateBioContextUser(const TMessage&);
        void StartRemoveBiometryContextUser(const TMessage&);

        bool BuildRequestContext(const TMessage&);
        void AddRequestContext();
        void AddEventException(const TString& code, const TString& text, const TString& fullText = {});

        TMessage CreateMessagePatchedWithExperiments(TMessage::TDirection, const TString& raw) const;

    private:
        NAppHost::TServiceContextPtr AhContext;

    public:
        NThreading::TPromise<void> Promise;

    private:
        TLogContext LogContext;
        TSourceMetrics Metrics;
        const NVoice::NExperiments::TExperiments& Experiments;
        NAliceProtocol::TSessionContext SessionContext;
        NAliceProtocol::TRequestContext RequestContext;
        NAliceProtocol::TRequestContext FallbackRequestContext;
        THolder<NCachalotProtocol::TActivationAnnouncementRequest> SmartActivationRequest;  // filled in BuildRequestContext (if need)
        bool RequestContextSended = false;
        TMatchedUserEventHandler MatchedUserEventHandler;
        NVoicetech::NSettings::TManagedSettings SettingsFromManager;
        TString RefMessageId;
        TString Mime;
        enum EStreamStatus {
            WaitStartStream,
            ProcessSpotter,
            ProcessStream, // <<< mean part stream after spotter here
            StreamEnded,
        } StreamStatus = WaitStartStream;
        bool AudioStreamStarted = false;
        bool HasSessionContext = false;
        bool HasManagerSettings = false;
        bool NeedFlush = false;
    };
}
