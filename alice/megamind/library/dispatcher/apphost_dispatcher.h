#pragma once

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>

#include <apphost/api/service/cpp/service_loop.h>

#include <util/generic/string.h>
#include <util/system/types.h>
#include <util/thread/pool.h>

namespace NAlice {

class IGlobalCtx;
class TConfig;

namespace NMegamind {

/** This class implements wrapper around AppHost Loop.
 */
class TAppHostDispatcher {
public:
    TAppHostDispatcher(IGlobalCtx& ctx);

    void Add(const TString& path, NAppHost::TAsyncAppService handler);
    void Add(const TString& path, NAppHost::TAppService handler);
    void Add(const TString& path, NNeh::TServiceFunction handler);

    void Start();
    void Stop();

private:
    struct TPort {
        TPort(const TConfig& config);
        ui16 Grpc;
        ui16 Http;
    };

    const TConfig& Config_;
    const TPort Port_;
    TRTLogger& Log_;
    NAppHost::TLoop Loop_;
    NMetrics::ISensors& Sensors_;
    THolder<IThreadPool> AsyncThreadPool_;
};

} // namespace NMegamind
} // namespace NAlice
