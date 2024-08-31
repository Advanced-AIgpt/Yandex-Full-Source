#pragma once

#include <alice/gproxy/library/gproxy/app_config.h>

#include <alice/gproxy/library/gproxy/subsystem_logging.h>
#include <alice/gproxy/library/gproxy/subsystem_metrics.h>
#include <alice/gproxy/library/gproxy/subsystem_apphost.h>
#include <alice/gproxy/library/gproxy/subsystem_grpc.h>
#include <alice/gproxy/library/gproxy/subsystem_admin.h>


namespace NGProxy {

class TApplication {
public:
    static int Run(int argc, const char **argv);

    ~TApplication() = default;

private:
    TApplication(const TApplicationConfig& config);

    int RunImpl();

    void InitSubsystems();

    void MainLoop();

private:    /* data */
    TApplicationConfig  Config;
    TLoggingSubsystem   Logging;
    TMetricsSubsystem   Metrics;
    TAppHostSubsystem   AppHost;
    TGrpcSubsystem      Grpc;
    TAdminSubsystem     Admin;
};


}   // namespace NGProxy
