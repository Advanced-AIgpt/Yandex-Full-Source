#pragma once

#include <alice/gproxy/library/gsetup/subsystem_logging.h>
#include <alice/gproxy/library/gsetup/subsystem_metrics.h>

#include <alice/gproxy/library/gsetup/config.pb.h>

#include <apphost/api/service/cpp/service.h>
#include <apphost/api/service/cpp/service_loop.h>

#include <alice/cuttlefish/library/apphost/admin_handle_listener.h>
#include <alice/cuttlefish/library/apphost/metrics_services.h>

#include "http_init.h"
#include "http_output.h"
#include "input.h"
#include "output.h"
#include "mmsetup.h"
#include "mm_rpc_setup.h"
#include "mm_rpc_output.h"

#include "request_meta_setup.h"

namespace NGProxy {


using namespace NGSetup;


class TServantSubsystem {
public:
    TServantSubsystem(const TServantConfig& config, TLoggingSubsystem& log, TMetricsSubsystem& metrics)
        : Config(config)
        , Log(log)
        , Metrics(metrics)
        , HttpInitHandler(Log)
        , HttpOutputHandler(Log)
        , InputHandler(Log)
        , OutputHandler(Log)
        , MMHandler(Log)
        , MMRpcHandler(Log)
        , MMRpcOutputHandler(Log)
        , RequestMetaHandler(Log)
    { }


    void Init() {
        Loop.SetAdminThreadCount(Config.GetAdminThreads());
        Loop.SetToolsThreadCount(Config.GetToolsThreads());
        Loop.EnableGrpc(Config.GetPort() + 1);
        Loop.SetGrpcThreadCount(Config.GetThreads());

        HttpInitHandler.Integrate(Config.GetPort(), Loop);
        HttpOutputHandler.Integrate(Config.GetPort(), Loop);
        InputHandler.Integrate(Config.GetPort(), Loop);
        OutputHandler.Integrate(Config.GetPort(), Loop);
        MMHandler.Integrate(Config.GetPort(), Loop);
        MMRpcOutputHandler.Integrate(Config.GetPort(), Loop);
        MMRpcHandler.Integrate(Config.GetPort(), Loop);
        RequestMetaHandler.Integrate(Config.GetPort(), Loop);
    }

    void Wait() {
        Loop.Loop(Config.GetThreads());
    }

    void Stop() {
        Loop.Stop();
    }

    inline TLoggingSubsystem& GetLogging() { return Log; }

    inline TMetricsSubsystem& GetMetrics() { return Metrics; }

private:
    TServantConfig     Config;
    TLoggingSubsystem& Log;
    TMetricsSubsystem& Metrics;

    TGSetupHttpInit    HttpInitHandler;
    TGSetupHttpOutput  HttpOutputHandler;
    TGSetupInput       InputHandler;
    TGSetupOutput      OutputHandler;
    TGSetupMM          MMHandler;
    TGSetupMMRpc       MMRpcHandler;
    TGSetupMMRpcOutput MMRpcOutputHandler;
    TGSetupRequestMeta RequestMetaHandler;

    ::NAppHost::TLoop  Loop;
};  // class TAdminSubsystem

}   // namespace NGProxy
