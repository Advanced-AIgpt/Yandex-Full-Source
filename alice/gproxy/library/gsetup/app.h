#pragma once

#include <alice/gproxy/library/gsetup/app_config.h>

#include <alice/gproxy/library/gsetup/subsystem_logging.h>
#include <alice/gproxy/library/gsetup/subsystem_metrics.h>
#include <alice/gproxy/library/gsetup/subsystem_servant.h>
#include <alice/gproxy/library/gsetup/subsystem_admin.h>


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
    TServantSubsystem   Servant;
    TAdminSubsystem     Admin;
};


}   // namespace NGProxy
