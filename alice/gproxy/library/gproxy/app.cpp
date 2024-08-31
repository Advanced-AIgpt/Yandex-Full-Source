#include "app.h"

#include <util/system/compiler.h>
#include <util/generic/yexception.h>
#include <util/system/mlock.h>


#include <alice/gproxy/library/protos/service.gproxy.pb.h>


namespace NGProxy {


TApplication::TApplication(const TApplicationConfig& config)
    : Config(config)
    , Logging(config.GetLoggingConfig())
    , AppHost(config.GetAppHostClientConfig(), Logging, Metrics)
    , Grpc(config.GetGrpcServerConfig(), Logging, Metrics, AppHost)
    , Admin(config.GetHttpServerConfig(), Logging, Metrics, Grpc)
{
}

int TApplication::Run(int argc, const char **argv) {
    TApplicationConfig config(argc, argv);
    TApplication App(config);
    return App.RunImpl();
}

int TApplication::RunImpl() {
    try {
        if (!Config.GetNoMLock()) {
            LockAllMemory(ELockAllMemoryFlag::LockCurrentMemory | ELockAllMemoryFlag::LockFutureMemory);
        }

        InitSubsystems();
        MainLoop();
    } catch (const yexception& err) {
        Cerr << "yexception: " << err.what() << Endl;
        return 1;
    } catch (const std::exception& err) {
        Cerr << "std::exception: " << err.what() << Endl;
        return 1;
    }
    return 0;
}


void TApplication::InitSubsystems() {
    Logging.Init();

    /* validate config */
    {
        if (Config.IsBad()) {
            ythrow yexception() << "config is bad, see stderr for more info";
        }
    }

    Metrics.Init();
    AppHost.Init();
    Admin.Init();
    Grpc.Init();
}


void TApplication::MainLoop() {
    Grpc.Wait();
    Admin.Wait();
    AppHost.Wait();
    Metrics.Wait();
    Logging.Wait();
}

}   // namespace NGProxy
