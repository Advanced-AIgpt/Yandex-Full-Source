#pragma once

#include <library/cpp/unistat/unistat.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>

namespace NAlice::NMusicMatchAdapter {
    class TUnistatCounterGuard {
    public:
        TUnistatCounterGuard(const TUnistatCounterGuard&) = delete;
        TUnistatCounterGuard& operator=(const TUnistatCounterGuard&) = delete;

        TUnistatCounterGuard(::NUnistat::IHole& hole)
            : Hole_(hole)
        {
            Hole_.PushSignal(1);
        }
        ~TUnistatCounterGuard() {
            Hole_.PushSignal(-1);
        }

    private:
        ::NUnistat::IHole& Hole_;
    };

    class TUnistatN {  // N - namespaced unistat (need for avoid conflict with global namespace TUnistat from library/cpp/unistat/unistat.h)
    public:
        TUnistatN();
        virtual ~TUnistatN() noexcept = default;

        TUnistatCounterGuard MusicMatchClientCounter();
        void OnCreateMusicMatchClient();

        TUnistatCounterGuard MusicMatchAudioConverterCounter();
        void OnMusicMatchCreateAudioConverter();
        void OnMusicMatchAudioConverterError();

        void OnMusicMatchReceivedAudioData(size_t size);
        void OnMusicMatchConvertedAudioData(size_t size);
        void OnMusicMatchSendedAudioData(size_t size);

        void OnMusicMatchRequestSuccess();
        void OnMusicMatchRequestError();
        void OnMusicMatchRequestCancel();

        void OnMusicMatchFinalResult();
        void OnMusicMatchClassifyingNotMusicResult();
        void OnMusicMatchClassifyingMusicResult();

        void OnMusicMatchWebsocketAnswerParseError();
        void OnMusicMatchWebsocketUpgradeResponseError();
        void OnMusicMatchWebsocketCloseError();
        void OnMusicMatchWebsocketNetworkError();
        void OnMusicMatchWebsocketWsError();
        void OnMusicMatchWebsocketTypedError();

    private:
        ::NUnistat::IHolePtr CreateMusicMatchClient_;
        ::NUnistat::IHolePtr MusicMatchClientCounter_;

        ::NUnistat::IHolePtr CreateMusicMatchAudioConverter_;
        ::NUnistat::IHolePtr MusicMatchAudioConverterCounter_;
        ::NUnistat::IHolePtr MusicMatchAudioConverterError_;

        ::NUnistat::IHolePtr MusicMatchReceivedAudioDataSize_;
        ::NUnistat::IHolePtr MusicMatchConvertedAudioDataSize_;
        ::NUnistat::IHolePtr MusicMatchSendedAudioDataSize_;

        // Requests are async
        // This is final results for requests
        ::NUnistat::IHolePtr MusicMatchRequestSuccess_;
        ::NUnistat::IHolePtr MusicMatchRequestError_;
        ::NUnistat::IHolePtr MusicMatchRequestCancel_;

        // Final responses
        ::NUnistat::IHolePtr MusicMatchFinalResultAll_;
        ::NUnistat::IHolePtr MusicMatchClassifyingNotMusicResult_;
        // Intermediate (not final) responses
        ::NUnistat::IHolePtr MusicMatchClassifyingMusicResult_;
        ::NUnistat::IHolePtr MusicMatchResultAll_;

        // Unexpected/bad answers from websocket
        ::NUnistat::IHolePtr MusicMatchWebsocketAnswerParseError_;

        ::NUnistat::IHolePtr MusicMatchWebsocketUpgradeResponseError_;
        ::NUnistat::IHolePtr MusicMatchWebsocketCloseError_;
        ::NUnistat::IHolePtr MusicMatchWebsocketNetworkError_;
        ::NUnistat::IHolePtr MusicMatchWebsocketWsError_;
        ::NUnistat::IHolePtr MusicMatchWebsocketTypedError_;
        ::NUnistat::IHolePtr MusicMatchWebsocketErrorAll_;
    };

    inline TUnistatN& Unistat() {
        return *Singleton<TUnistatN>();
    }

}
