#pragma once

#include "unistat.h"

#include <alice/cuttlefish/library/music_match/base/interface.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/asconv/threaded_converter.h>
#include <voicetech/library/messages/message.h>
#include <voicetech/library/ws_server/error.h>
#include <voicetech/library/ws_server/ws_server.h>

#include <library/cpp/neh/asio/executor.h>
#include <library/cpp/threading/atomic/bool.h>

#include <util/generic/ptr.h>

namespace NAlice::NMusicMatchAdapter {
    class TMusicMatchClient : public NVoicetech::TWsHandler {
    public:
        TMusicMatchClient();
        ~TMusicMatchClient();

        void SendStreamRequest(const NMusicMatch::NProtobuf::TStreamRequest& streamRequest);
        void EnableAudioConverter(const TString& fromMime, const TString& toMime);
        void SafeCauseError(const TString& error);
        void SafeClose();

        void SetFinishPromise(NThreading::TPromise<void>& promise);

    protected:
        virtual void OnInitResponse(const NMusicMatch::NProtobuf::TInitResponse& initResponse) = 0;
        virtual void OnStreamResponse(const NMusicMatch::NProtobuf::TStreamResponse& streamResponse) = 0;
        virtual void OnClosed() = 0;
        virtual void OnAnyError(const TString& error) = 0;

    private:
        // WebSocket handlers
        void OnUpgradeResponse(const THttpParser&, const TString& error) override;
        void OnTextMessage(const char* data, size_t size) override;
        void OnBinaryMessage(const void* data, size_t size) override;
        void OnCloseMessage(ui16 code, const TString& reason) override;
        void OnError(const NVoicetech::TNetworkError&) override;
        void OnError(const NRfc6455::TWsError&) override;
        void OnError(const NVoicetech::TTypedError&) override;

        // Client specific
        void ProcessOnStreamResponse(const TString& rawJsonResult, bool isFinish);
        void ProcessSendCloseMessage();

        void SendDataFromAudioConverter(bool isEof);

        void OnFail(const TString& error);

    protected:
        NAtomic::TBool RequestUpgraded_ = false;
        NAtomic::TBool Finished_ = false;
        NAtomic::TBool WebsocketClosed_ = false;
        NAtomic::TBool WebsocketCloseSended_ = false;

        NAtomic::TBool RequestSucceeded_ = false;
        NAtomic::TBool RequestCanceled_ = false;

    private:
        class TAudioConverter : public NVoicetech::NASConv::TThreadedConverter {
        public:
            TAudioConverter(
                const TString& fromMime,
                const TString& toMime,
                const NVoicetech::NASConv::TConverterOptions& options,
                TOnOutputCallback onOutputCallback
            );

        private:
            TUnistatCounterGuard UnistatCounterGuard_;  // count audio converters (for detect leaks)
        };

        THolder<TAudioConverter> AudioConverter_ = nullptr;
        TVector<char> AudioConverterBuffer_;

        TUnistatCounterGuard UnistatCounterGuard_;  // count clients (for detect leaks)

        TMaybe<NThreading::TPromise<void>> FinishPromise_ = Nothing();
    };
}
