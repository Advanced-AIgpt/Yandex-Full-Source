#pragma once

#include <alice/bass/libs/globalctx/fwd.h>

#include <library/cpp/monlib/dynamic_counters/page.h>

#include <util/generic/ptr.h>
#include <library/cpp/deprecated/atomic/atomic.h>

class TAvatarsMap;
class TBassServer;
class TConfig;
class IScheduler;

class TSourcesRegistry;

namespace NMonitoring {
class TMonService2;
class TMetricRegistry;
} // NMonitoring

namespace NBASS {

class TCacheData;
class TCacheManager;

namespace NAutomotive {
class TFMRadioDatabase;
} // NAutomotive

} // NBASS

// Singleton class for keeping all application wide things like
// HTTP server, thread pool, counters, etc.
class TApplication {
private:
    using TMonService = NMonitoring::TMonService2;

private:
    NBASS::TGlobalContextPtr GlobalContext;

public:
    TApplication(int argc, const char** argv);
    ~TApplication();

    // Get current global instance.
    static TApplication* GetInstance() {
        return Instance;
    }

    // Run loop and return exit code.
    void Run();

    // Initiate gracefully shutdown, loop will terminate soon.
    void Shutdown();

    // Return true if graceful shutdown was initiated.
    bool IsShutdown() const {
        return IsShutdownInitiated;
    }

    // Reopen logs for rotation
    void ReopenLogs();

    // Return application config.
    const TConfig& GetConfig() const;

    const TAvatarsMap& Avatars() const;

    // Low-level, please use TContext::GetSources() to create source requests
    const TSourcesRegistry& GetSources() const;

    void ReloadYaStrokaFixList();
    void ReloadBnoApps();

private:
    void InitPreTVM2();
    void InitPreTVM2Alice();
    void InitTVM2();
    void InitPostTVM2();
    void InitPostTVM2Alice();

private:
    static TApplication* Instance;

private:
    std::unique_ptr<NMonitoring::TMonService2> MonService;
    std::unique_ptr<TBassServer> HttpServer;

    TAtomic IsShutdownInitiated = false;
    bool IsRunning = false;
    bool SkipWaitingFirstRun = false;
};
