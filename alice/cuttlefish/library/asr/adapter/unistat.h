#pragma once

#include <library/cpp/unistat/unistat.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>

namespace NAlice::NAsrAdapter {
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

        TUnistatCounterGuard Asr2ViaAsr1ClientCounter() {
            return TUnistatCounterGuard(*Asr2ViaAsr1ClientCounter_);
        }
        TUnistatCounterGuard Asr1ClientCounter() {
            return TUnistatCounterGuard(*Asr1ClientCounter_);
        }
        void OnCreateAsr1Client();
        static const int ConnectFailed = 600;
        static const int ParseHttpResponseFailed = 601;
        void OnAsr1Error(int code = 0);
        static const int SpotterFailInit = 740;
        static const int SpotterInvalidTextResponse = 742;
        static const int SpotterRequestFailed = 777;
        static const int SpotterErrorDeadline = 708;
        static const int SpotterCancel = 999;
        void OnAsr1SpotterError(int code = 0);
        void OnAsr1SpotterCancel();
        void OnReceiveFromAppHostReqRaw(size_t size);
        void OnReceiveFromAppHostRaw(size_t size);
        void OnReceiveFromAsrRaw(size_t size);
        void OnSendToAppHostRaw(size_t size);
        void OnSendToAsrRaw(size_t size);

    private:
        ::NUnistat::IHolePtr Asr2ViaAsr1ClientCounter_;
        ::NUnistat::IHolePtr CreateAsr1Client_;
        ::NUnistat::IHolePtr Asr1ClientCounter_;

        ::NUnistat::IHolePtr Asr1ErrorAll_;
        ::NUnistat::IHolePtr Asr1ErrorBadMessage_;
        ::NUnistat::IHolePtr Asr1ErrorConnect_;
        ::NUnistat::IHolePtr Asr1ErrorParseHttpResponse_;
        ::NUnistat::IHolePtr Asr1ErrorInvalidParams_;
        ::NUnistat::IHolePtr Asr1ErrorTimeout_;
        ::NUnistat::IHolePtr Asr1ErrorInternal_;
        ::NUnistat::IHolePtr Asr1ErrorUnspecific_;

        ::NUnistat::IHolePtr Asr1SpotterErrorAll_;
        ::NUnistat::IHolePtr Asr1SpotterErrorBadMessage_;
        ::NUnistat::IHolePtr Asr1SpotterErrorInvalidParams_;
        ::NUnistat::IHolePtr Asr1SpotterErrorTimeout_;
        ::NUnistat::IHolePtr Asr1SpotterErrorInternal_;
        ::NUnistat::IHolePtr Asr1SpotterErrorDeadline_;
        ::NUnistat::IHolePtr Asr1SpotterErrorRequest_;
        ::NUnistat::IHolePtr Asr1SpotterErrorUnspecific_;

        ::NUnistat::IHolePtr Asr1SpotterCancel_;

        ::NUnistat::IHolePtr RecvFromAppHostReqRawBytes_;
        ::NUnistat::IHolePtr RecvFromAppHostRawBytes_;
        ::NUnistat::IHolePtr RecvFromAppHostItems_;
        ::NUnistat::IHolePtr RecvFromAsrRawBytes_;
        ::NUnistat::IHolePtr SendToAppHostRawBytes_;
        ::NUnistat::IHolePtr SendToAppHostItems_;
        ::NUnistat::IHolePtr SendToAsrRawBytes_;
    };

    inline TUnistatN& Unistat() {
        return *Singleton<TUnistatN>();
    }

}
