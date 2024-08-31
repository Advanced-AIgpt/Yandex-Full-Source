#include "app.h"

#include <util/system/compiler.h>
#include <util/generic/yexception.h>
#include <util/system/mlock.h>


namespace NGProxy {


TApplication::TApplication(const TApplicationConfig& config)
    : Config(config)
    , Logging(config.GetLoggingConfig())
    , Servant(config.GetServantConfig(), Logging, Metrics)
    , Admin(config.GetHttpServerConfig(), Logging, Metrics, Servant)
{ }


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
    Servant.Init();
    Admin.Init();
}


void TApplication::MainLoop() {
    Servant.Wait();
    Admin.Wait();
    Metrics.Wait();
    Logging.Wait();
}

}   // namespace NGProxy
