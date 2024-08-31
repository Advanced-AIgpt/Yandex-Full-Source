#pragma once

#include "asr1_client.h"
#include "protocol_convertor.h"
#include "unistat.h"

#include <alice/cuttlefish/library/asr/base/interface.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>
#include <util/generic/maybe.h>
#include <util/system/spinlock.h>
#include <voicetech/library/ws_server/http_client.h>

#include <atomic>
#include <queue>
#include <memory>

namespace NAlice::NAsrAdapter {
    struct TAsrInfo {
        TString Topic;
        TString TopicVersion;
        TString ServerVersion;
        bool UseFakeTopic = false;
    };

    class TAsr2ViaAsr1Client : public TProtocolConvertor, public NAsr::TInterface {
    public:
        TAsr2ViaAsr1Client(
            NVoicetech::THttpClient& httpClient,
            TIntrusivePtr<TCallbacks>& callbacks,
            const NCuttlefish::TLogContext& logContextForCallbacks,
            const NCuttlefish::TLogContext& logContext,
            TDuration spotterDeadline);

        void StartRequest(
            const TString& asrUrl,
            const NAsr::NProtobuf::TInitRequest&,
            const TAsrInfo& asrInfo,
            bool ignoreParsingProtobufError
        );

    private:
        void InitImpl(
            const NAsr::NProtobuf::TInitRequest&,
            const TAsrInfo& asrInfo,
            bool ignoreParsingProtobufError
        );

    public:
        // impl TAsrInterface
        bool ProcessAsrRequest(const NAsr::NProtobuf::TRequest&) override;
        void CauseError(const TString& error) override;
        void Close() override;

        void SafeInjectAsrResponse(NAsr::NProtobuf::TResponse&&);

        // return false if yet not ready handler EOU result (spotter yet not ready)
        bool OnMainEou();

        void OnSpotterInitResponse(const YaldiProtobuf::InitResponse&);
        void OnSpotterAddDataResponse(const YaldiProtobuf::AddDataResponse&);
        void OnSpotterClosed(int errorCode = 0);

    protected:
        static YaldiProtobuf::InitRequest YaldiInitRequest(const NAsr::NProtobuf::TInitRequest&);

        class TMainAsr1Client : public TProtocolConvertor, public TAsr1Client {
        public:
            TMainAsr1Client(
                const YaldiProtobuf::InitRequest&,
                const TIntrusivePtr<NAsr::TInterface::TCallbacks>&,
                const TIntrusivePtr<TAsr2ViaAsr1Client>&,
                const TAsrInfo& asrInfo,
                const NAlice::NCuttlefish::TLogContext&
            );

            // unsafe method for inject outsider responses (not from backend), - obviously errors
            void OnAsrResponse(const NAsr::NProtobuf::TResponse&);
            void ResumeResponses();
            void ResetResponses();
            void SetValidationSuccess() {
                ValidationSuccess_ = true;
            }
            TString RtLogToken() const {
                return RtLogChild_.Token();
            }
            void SetSingleUtterance(bool b) {
                SingleUtterance_ = b;
            }

        private:
            // impl. asr1_client callbacks
            void OnSend() override;
            void OnInitResponse(const YaldiProtobuf::InitResponse&) override;
            void OnAddDataResponse(const YaldiProtobuf::AddDataResponse&) override;
            void OnClosed() override;
            void OnAnyError(const TString&, bool fastError, int errorCode) override;

            void CheckResponseCode(int responseCode);
            TIntrusivePtr<TAsr2ViaAsr1Client> ReleaseAsr() {
                TIntrusivePtr<TAsr2ViaAsr1Client> asr;
                asr.Swap(Asr_);
                return asr;
            }

        private:
            TIntrusivePtr<NAsr::TInterface::TCallbacks> Asr2Callbacks_;
            TIntrusivePtr<TAsr2ViaAsr1Client> Asr_;
            TAsrInfo AsrInfo_;
            NAlice::NCuttlefish::TLogContext Log_;
            NCuttlefish::TRTLogActivation RtLogChild_;
            TInstant NextHeartBeatTime_;
            bool ReceiveEou_ = false;
            using TSuspendedResponsesQueue = std::queue<std::unique_ptr<NAsr::NProtobuf::TResponse>>;
            std::unique_ptr<TSuspendedResponsesQueue> SuspendResponses_;
            bool PostponedClose_ = false;
            TString ResponseFakeTopic_;
            bool ValidationSuccess_ = false;
            bool SingleUtterance_ = false;
        };

        class TSpotterAsr1Client : public TProtocolConvertor, public TAsr1Client {
        public:
            TSpotterAsr1Client(
                NAsio::TIOService&,
                const YaldiProtobuf::InitRequest&,
                const TIntrusivePtr<TAsr2ViaAsr1Client>&,
                const NAlice::NCuttlefish::TLogContext&
            );

            void SendLastChunk();
            void SetDeadline(TDuration);
            TString RtLogToken() const {
                return RtLogChild_.Token();
            }

            void SafeClose(NAsio::TIOService& ioService) override {
                Closing_ = true;
                TAsr1Client::SafeClose(ioService);
            }

        private:
            void OnDeadline();
            void Close() {
                Cancel();
            }

            // impl. asr1_client callbacks
            void OnInitResponse(const YaldiProtobuf::InitResponse& initResponse) override;
            void OnAddDataResponse(const YaldiProtobuf::AddDataResponse& addDataResponse) override;
            void OnClosed() override;
            void OnAnyError(const TString&, bool fastError, int errorCode) override;

            TIntrusivePtr<TAsr2ViaAsr1Client> ReleaseAsr() {
                TIntrusivePtr<TAsr2ViaAsr1Client> asr;
                asr.Swap(Asr_);
                return asr;
            }

        private:
            NAsio::TIOService& IOService_;
            TIntrusivePtr<TAsr2ViaAsr1Client> Asr_;
            NAlice::NCuttlefish::TLogContext Log_;
            NCuttlefish::TRTLogActivation RtLogChild_;
            std::unique_ptr<NAsio::TDeadlineTimer> ValidationDeadlineTimer_;
            std::atomic_bool Closing_{false};
        };

    private:
        NVoicetech::TUpgradedHttpHandlerRef Handler() {
            auto client = GetMainAsrClient();
            return NVoicetech::TUpgradedHttpHandlerRef(client.Get());
        }

        NVoicetech::TUpgradedHttpHandlerRef SpotterHandler() {
            auto client = GetSpotterAsrClient();
            return NVoicetech::TUpgradedHttpHandlerRef(client.Get());
        }

        TString MainRtLogToken() {
            auto client = GetMainAsrClient();
            if (client) {
                return client->RtLogToken();
            }
            return {};
        }

        TString SpotterRtLogToken() {
            auto client = GetSpotterAsrClient();
            if (client) {
                return client->RtLogToken();
            }
            return {};
        }

    private:
        TIntrusivePtr<TMainAsr1Client> GetMainAsrClient() {
            with_lock (ClientsLock_) {
                return Asr1Client_;
            }
        }
        TIntrusivePtr<TSpotterAsr1Client> GetSpotterAsrClient() {
            with_lock (ClientsLock_) {
                return SpotterAsr1Client_;
            }
        }
        void SetMainAsrClient(TIntrusivePtr<TMainAsr1Client> client) {
            with_lock (ClientsLock_) {
                Asr1Client_.Swap(client);
            }
        }
        void SetSpotterAsrClient(TIntrusivePtr<TSpotterAsr1Client> client) {
            with_lock (ClientsLock_) {
                SpotterAsr1Client_.Swap(client);
            }
        }

        char MagicKey_[16] = "424344454547484";  // marker for search log frames ptr's in coredumps
        TString MessageId_;
        NVoicetech::THttpClient& HttpClient_;
        NAsio::TIOService& IOService_;
        NAlice::NCuttlefish::TLogContext CallbacksLog_;
        NAlice::NCuttlefish::TLogContext Log_;

        TAdaptiveLock ClientsLock_; // clients can be accessed from misc threads
        TIntrusivePtr<TMainAsr1Client> Asr1Client_;
        TIntrusivePtr<TSpotterAsr1Client> SpotterAsr1Client_;

        size_t ChunksSendedToSpotterClient_ = 0;
        TString SpotterPhrase_;
        NAtomic::TBool SpotterClosed_ = false;
        const TDuration SpotterDeadline_;
        TMaybe<bool> ValidationResult_;
        static const ui32 MainClientNum_ = 0;
        static const ui32 SpotterClientNum_ = 1;
        TUnistatCounterGuard UnistatCounterGuard_;  // count clients (for detect leaks)
        TAsrInfo AsrInfo_;
    };
}
