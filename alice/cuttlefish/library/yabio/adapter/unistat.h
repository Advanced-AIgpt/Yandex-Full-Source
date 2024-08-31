#pragma once

#include <library/cpp/unistat/unistat.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>

namespace NAlice::NYabioAdapter {
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

        TUnistatCounterGuard YabioClientCounter() {
            return TUnistatCounterGuard(*YabioClientCounter_);
        }
        void OnCreateYabioClient();

        void OnYabioError(int code = 0);
        void OnYabioWarning();

    private:
        ::NUnistat::IHolePtr CreateYabioClient_;
        ::NUnistat::IHolePtr YabioClientCounter_;

        ::NUnistat::IHolePtr YabioErrorAll_;
        ::NUnistat::IHolePtr YabioErrorBadMessage_;
        ::NUnistat::IHolePtr YabioErrorInvalidParams_;
        ::NUnistat::IHolePtr YabioErrorTimeout_;
        ::NUnistat::IHolePtr YabioErrorInternal_;
        ::NUnistat::IHolePtr YabioErrorUnspecific_;

        ::NUnistat::IHolePtr YabioSpotterErrorAll_;
        ::NUnistat::IHolePtr YabioSpotterErrorBadMessage_;
        ::NUnistat::IHolePtr YabioSpotterErrorInvalidParams_;
        ::NUnistat::IHolePtr YabioSpotterErrorTimeout_;
        ::NUnistat::IHolePtr YabioSpotterErrorInternal_;
        ::NUnistat::IHolePtr YabioSpotterErrorDeadline_;
        ::NUnistat::IHolePtr YabioSpotterErrorRequest_;
        ::NUnistat::IHolePtr YabioSpotterErrorUnspecific_;

        ::NUnistat::IHolePtr YabioWarning_;
    };

    inline TUnistatN& Unistat() {
        return *Singleton<TUnistatN>();
    }

}
