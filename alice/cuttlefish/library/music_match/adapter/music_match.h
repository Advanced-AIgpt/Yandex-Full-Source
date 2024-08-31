#pragma once

#include "music_match_client.h"

#include <alice/cuttlefish/library/music_match/base/interface.h>

#include <alice/cuttlefish/library/logging/log_context.h>

namespace NAlice::NMusicMatchAdapter {
    class TMusicMatch : public NMusicMatch::TInterface {
    public:
        TMusicMatch(
            TIntrusivePtr<TCallbacks>& callbacks,
            const NCuttlefish::TLogContext&
        );

        // impl TMusicMatchInterface
        void ProcessContextLoadResponse(const NMusicMatch::NProtobuf::TContextLoadResponse& contextLoadResponse) override;
        void ProcessSessionContext(const NMusicMatch::NProtobuf::TSessionContext& sessionContext) override;
        void ProcessTvmServiceTicket(const TString& tvmServiceTicket) override;
        void ProcessInitRequest(const NMusicMatch::NProtobuf::TInitRequest& initRequest) override;
        void ProcessStreamRequest(const NMusicMatch::NProtobuf::TStreamRequest& streamRequest) override;
        void CauseError(const TString& error) override;
        void Close() override;

        void SetFinishPromise(NThreading::TPromise<void>& promise) override;

        NVoicetech::TWsHandlerRef Handler() {
            return MusicMatchClient_.Get();
        }
        TString InitClientAndGetHeaders() const;

    protected:
        class TMyMusicMatchClient : public TMusicMatchClient {
        public:
            TMyMusicMatchClient(
                TIntrusivePtr<NMusicMatch::TInterface::TCallbacks>& callbacks,
                NRTLog::TRequestLoggerPtr rtLogger
            );

            TString GetRtLogToken() const;

        protected:
            void OnInitResponse(const NMusicMatch::NProtobuf::TInitResponse& initResponse) override;
            void OnStreamResponse(const NMusicMatch::NProtobuf::TStreamResponse& streamResponse) override;
            void OnClosed() override;
            void OnAnyError(const TString& error) override;

        private:
            TIntrusivePtr<NMusicMatch::TInterface::TCallbacks> MusicMatchCallbacks_;
            NCuttlefish::TRTLogActivation RtLogChild_;
        };

        TMaybe<NMusicMatch::NProtobuf::TContextLoadResponse> ContextLoadResponse_ = Nothing();
        TMaybe<NMusicMatch::NProtobuf::TSessionContext> SessionContext_ = Nothing();
        TMaybe<TString> TvmServiceTicket_ = Nothing();
        TMaybe<NMusicMatch::NProtobuf::TInitRequest> InitRequest_ = Nothing();
        TIntrusivePtr<TMyMusicMatchClient> MusicMatchClient_;

        NCuttlefish::TLogContext Log_;
    };
}
