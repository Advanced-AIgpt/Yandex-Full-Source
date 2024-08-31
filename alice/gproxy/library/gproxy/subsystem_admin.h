#pragma once

#include <alice/gproxy/library/gproxy/subsystem_logging.h>
#include <alice/gproxy/library/gproxy/subsystem_metrics.h>
#include <alice/gproxy/library/gproxy/subsystem_grpc.h>

#include <alice/gproxy/library/gproxy/config.pb.h>

#include <apphost/api/service/cpp/service.h>
#include <apphost/api/service/cpp/service_loop.h>

#include <alice/cuttlefish/library/apphost/admin_handle_listener.h>
#include <alice/cuttlefish/library/apphost/metrics_services.h>


namespace NGProxy {

/**
 *  @brief admin subsystem for gproxy
 *  Provides the following handlers:
 *    - /admin?action=shutdown
 *    - /admin?action=logrotate
 *    - /_solomon
 */
class TAdminSubsystem {
public:
    TAdminSubsystem(const THttpServerConfig& config, TLoggingSubsystem& log, TMetricsSubsystem& metrics, TGrpcSubsystem& grpc)
        : Config(config)
        , Log(log)
        , Metrics(metrics)
        , Grpc(grpc)
        , Listener(config.GetPort())
    { }


    void Init() {
        Loop.SetAdminThreadCount(1);
        Loop.SetToolsThreadCount(1);
        Loop.Add(Config.GetPort(), "/unistat", NAlice::NCuttlefish::GolovanMetricsService);
        Loop.SetAdminHandleListener(&Listener);
        //[APPHOSTSUPPORT-358] Crutch in order to '/admin?action=... ' work.
        Loop.Add(Config.GetPort(), "/never_use_this_route", [](NAppHost::TServiceContextPtr) {
            return NThreading::NewPromise<void>().GetFuture();
        });
        Loop.ForkLoop(1);
    }

    void Wait() { }

    void Stop() {
        Loop.Stop();
    }

protected:
    void OnAdmin(const NNeh::IRequestRef& req);

private:
    THttpServerConfig  Config;
    TLoggingSubsystem& Log;
    TMetricsSubsystem& Metrics;
    TGrpcSubsystem&    Grpc;

    ::NAppHost::TLoop  Loop;

    NAlice::NCuttlefish::TAdminHandleListener Listener;
};  // class TAdminSubsystem

}   // namespace NGProxy
