#include "apphost.h"

#include <alice/hollywood/library/dispatcher/common_handles/hw_service_handles/hw_service.h>
#include <alice/hollywood/library/dispatcher/common_handles/scenario_handles/scenario.h>
#include <alice/hollywood/library/dispatcher/common_handles/service_handles/service.h>
#include <alice/hollywood/library/dispatcher/common_handles/split_web_search/split_web_search.h>
#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>
#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/framework/core/scenario_factory.h>

#include <alice/library/metrics/util.h>

#include <library/cpp/sighandler/async_signals_handler.h>

namespace NAlice::NHollywood {

namespace {

class TOngoingRequestsCounter {
public:
    TOngoingRequestsCounter(NMetrics::ISensors& sensors, NMonitoring::TLabels&& labels)
        : Sensors_(sensors)
        , Labels_(std::move(labels))
    {
        Labels_.Add("name", "ongoing_requests");
        Sensors_.AddIntGauge(Labels_, 1);
    }

    ~TOngoingRequestsCounter() noexcept(false) {
        Sensors_.AddIntGauge(Labels_, -1);
    }

private:
    NMetrics::ISensors& Sensors_;
    NMonitoring::TLabels Labels_;
};

} // namespace

TApphostDispatcher::TApphostDispatcher(const TConfig_TAppHostConfig& config,
                                       const TSet<TString>& scenarios,
                                       const TScenarioRegistry& registry,
                                       const THwServiceRegistry& serviceRegistry,
                                       TGlobalContext& globalContext,
                                       bool enableCommonHandles)
    : Config_(config)
    , GlobalContext_(globalContext)
{
    auto& baseLogger = GlobalContext_.BaseLogger();

    const auto port = Config_.GetPort();
    Y_ENSURE(port <= Max<uint16_t>());
    LOG_INFO(baseLogger) << "Base port " << port;

    const auto grpcPort = port + 1;
    Y_ENSURE(grpcPort <= Max<uint16_t>());
    LOG_INFO(baseLogger) << "GRPC port " << grpcPort;

    Loop_.EnableGrpc(grpcPort, {
        .ReusePort = true,
        .Threads = static_cast<int>(Config_.GetThreads())
    });

    // Scenario handles (old flow)
    for (const auto& name : scenarios) {
        const TScenario& scenario = registry.GetScenario(name);
        for (const auto& handle : scenario.Handles()) {
            const auto handlePath = JoinFsPaths("/", name, handle->Name());
            if (NHollywoodFw::NPrivate::TScenarioFactory::Instance().LinkOldScenario(scenario, handle->Name())) {
                LOG_INFO(baseLogger) << "grpc path " << handlePath << " exists twice, link it to new scenario";
                continue;
            }
            Loop_.Add(port, handlePath, [&scenario, &handle, &globalContext](NAppHost::IServiceContext& ctx) {
                TOngoingRequestsCounter counter(globalContext.Sensors(), ScenarioLabels(*handle));
                DispatchScenarioHandle(scenario, *handle, globalContext, ctx);
            });
            LOG_INFO(baseLogger) << "Added grpc path " << handlePath;
        }
    }

    // Scenario handles (new flow, collect only non-empty nodes)
    TVector<NHollywoodFw::NPrivate::THandlerInfo> allHandlers;
    NHollywoodFw::NPrivate::TScenarioFactory::Instance().CollectAllScenariosHandlers(allHandlers);
    for (const auto& it: allHandlers) {
        const auto handlePath = JoinFsPaths("/", it.Scenario->GetName(), it.HandlerName);
        Loop_.Add(port, handlePath, [it, &globalContext](NAppHost::IServiceContext& ctx) {
            TOngoingRequestsCounter counter(globalContext.Sensors(), ScenarioLabels(*it.Scenario, it.HandlerName));
            NHollywoodFw::NPrivate::TScenarioFactory::Instance().DispatchScenarioHandle(*it.Scenario, it.HandlerName, globalContext, ctx);
        });
        LOG_INFO(baseLogger) << "Added grpc path " << handlePath;
    }

    // Hollywood service handles
    for (const auto& handle : serviceRegistry.GetHandles()) {
        const auto handlePath = JoinFsPaths("/", handle->Name());
        Loop_.Add(port, handlePath, [&handle, &globalContext](NAppHost::IServiceContext& ctx) {
            TOngoingRequestsCounter counter(globalContext.Sensors(), HwServiceLabels(*handle));
            DispatchHwServiceHandle(*handle, globalContext, ctx);
        });
        LOG_INFO(baseLogger) << "Added grpc path " << handlePath;
    }

    // AppHost Service handles
    const std::tuple<TString, EMiscHandle, void (*)(NAppHost::IServiceContext&, TGlobalContext&)> apphostServiceHandlers[] = {
        {UTILITY_PATH, EMiscHandle::UTILITY, &UtilityHandler},
        // Temporary crutch for backward compatibility
        {VERSION_PATH,      EMiscHandle::VERSION, &GetVersionAppHostHandle},
        {VERSION_JSON_PATH, EMiscHandle::VERSION, &GetVersionAppHostHandle},
    };
    for (const auto& [path, miscHandle, handler] : apphostServiceHandlers) {
        Loop_.Add(port, path, [&globalContext, miscHandle = miscHandle, handler = handler](NAppHost::IServiceContext& ctx) {
            TOngoingRequestsCounter counter(globalContext.Sensors(), MiscLabels(miscHandle));
            handler(ctx, globalContext);
        });
        LOG_INFO(baseLogger) << "Added grpc path " << path;
    }

    // HTTP Service handles
    const std::tuple<TString, EMiscHandle, void (*)(TGlobalContext& globalContext, const NNeh::IRequestRef& req)> httpServiceHandlers[] = {
        {FAST_DATA_VERSION_PATH, EMiscHandle::FAST_DATA_VERSION, &GetFastDataVersionHandle},
        {PING_PATH,              EMiscHandle::PING,              &PingHandle},
        {RELOAD_FAST_DATA_PATH,  EMiscHandle::FAST_DATA_RELOAD,  &ReloadFastDataHandle},
        {SOLOMON_PATH,           EMiscHandle::SOLOMON,           &DumpSolomonCountersHandle},
        {VERSION_PATH,           EMiscHandle::VERSION,           &GetVersionHttpHandle},
        {VERSION_JSON_PATH,      EMiscHandle::VERSION,           &GetVersionHttpHandle},
        {REOPEN_LOGS_PATH,       EMiscHandle::REOPEN_LOGS,       &ReopenLogsHandle},
    };
    for (const auto& [path, miscHandle, handler] : httpServiceHandlers) {
        Loop_.Add(port, path, [&globalContext, miscHandle = miscHandle, handler = handler](const NNeh::IRequestRef& req) {
            TOngoingRequestsCounter counter(globalContext.Sensors(), MiscLabels(miscHandle));
            handler(globalContext, req);
        });
        LOG_INFO(baseLogger) << "Added http path " << path;
    }

    // Common Alice handles
    if (enableCommonHandles) {
        const std::tuple<TString, EMiscHandle, void (*)(NAppHost::IServiceContext&, TGlobalContext&)> apphostCommonHandlers[] = {
            {SPLIT_WEB_SEARCH_PATH, EMiscHandle::SPLIT_WEB_SEARCH, &SplitWebSearch},
        };
        for (const auto& [path, miscHandle, handler] : apphostCommonHandlers) {
            Loop_.Add(port, path, [&globalContext, miscHandle = miscHandle, handler = handler](NAppHost::IServiceContext& ctx) {
                TOngoingRequestsCounter counter(globalContext.Sensors(), MiscLabels(miscHandle));
                handler(ctx, globalContext);
            });
            LOG_INFO(baseLogger) << "Added grpc path " << path;
        }
    }
}

void TApphostDispatcher::Dispatch() {
    auto& baseLogger = GlobalContext_.BaseLogger();

    const auto shutdown = [this](int) {
        Loop_.Stop();  // non-blocking
    };

    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);

    LOG_INFO(baseLogger) << "Apphost dispatcher is starting";
    Loop_.Loop(Config_.GetThreads());
    LOG_INFO(baseLogger) << "Apphost dispatcher has stopped";
}

} // namespace NAlice::NHollywood
