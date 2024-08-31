#pragma once

#include "service.h"

#include <alice/matrix/library/clients/tvm_client/tvm_client.h>
#include <alice/matrix/library/config/config.pb.h>
#include <alice/matrix/library/logging/event_log.h>
#include <alice/matrix/library/metrics/metrics.h>
#include <alice/matrix/library/mlock/mlock.h>
#include <alice/matrix/library/version/version.h>

#include <alice/cuttlefish/library/apphost/admin_handle_listener.h>

#include <apphost/api/service/cpp/service.h>
#include <apphost/api/service/cpp/service_loop.h>

#include <library/cpp/proto_config/load.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/system/env.h>
#include <util/system/getpid.h>
#include <util/system/hostname.h>


namespace NMatrix::NApplication {

namespace NPrivate {

template <typename TServices>
class TAdminHandleListener : public NAlice::NCuttlefish::TAdminHandleListener {
public:
    explicit TAdminHandleListener(ui16 port, TServices* services)
        : NAlice::NCuttlefish::TAdminHandleListener(port)
        , Services_(services)
    {
    }

    void OnShutdown() override {
        Services_->Suspend();
    }

private:
    TServices* Services_ = nullptr;
};

void ConfigureLogger(const TLoggerSettings& config);
void ConfigureNeh(const TNehSettings& config);
NAppHost::TLoop& ConfigureLoop(
    NAppHost::TLoop& loop,
    const TServerSettings& config
);

TRtLogClient CreateRtLogClient(const TRtLogSettings& config);
NYdb::TDriver CreateYDBDriver(const TYDBClientCommonSettings& config);

template <
    const char* ServiceName,
    typename TServicesCommonContextBuilder,
    typename ...TServices
>
bool RunLoop(const typename TServicesCommonContextBuilder::TCommonContextType::TConfigType& config) {
    ConfigureLogger(config.GetLog());
    auto logFrame = SpawnLogFrame();

    try {
        TryMlockAndReport(config.GetLockAllMemory(), logFrame);

        if constexpr (requires {config.GetNeh();}) {
            NPrivate::ConfigureNeh(config.GetNeh());
        }

        TServicesCommonContextBuilder servicesCommonContextBuilder;
        servicesCommonContextBuilder.WithConfig(config);

        TMaybe<TRtLogClient> rtLogClient = Nothing();
        TMaybe<TTvmClient> tvmClient = Nothing();
        TMaybe<NYdb::TDriver> ydbDriver = Nothing();

        if constexpr (requires {config.GetRtLog();}) {
            rtLogClient.ConstructInPlace(NPrivate::CreateRtLogClient(config.GetRtLog()));
            servicesCommonContextBuilder.WithRtLogClient(rtLogClient.GetRef());
        }

        if constexpr (requires {config.GetTvmClient();}) {
            tvmClient.ConstructInPlace(config.GetTvmClient());
            servicesCommonContextBuilder.WithTvmClient(tvmClient.GetRef());
        }

        if constexpr (requires {config.GetYDBClientCommon();}) {
            ydbDriver.ConstructInPlace(NPrivate::CreateYDBDriver(config.GetYDBClientCommon()));
            servicesCommonContextBuilder.WithYDBDriver(ydbDriver.GetRef());
        }

        auto servicesCommonContext = servicesCommonContextBuilder.Build();

        TString hostName = "unknown_hostname";
        try {
            hostName = HostName();
        } catch (...) {
            hostName = TString::Join("Failed to get hostname: ", CurrentExceptionMessage());
        }

        logFrame->LogEvent(
            NEvClass::TMatrixServerStart(
                hostName,
                GetVersion(),
                GetPID(),

                config.GetServer().GetHttpPort(),
                config.GetServer().GetGrpcPort(),

                config.GetServer().GetLoopThreads(),
                config.GetServer().GetGrpcThreads(),

                config.GetServer().GetAdminThreads(),
                config.GetServer().GetToolsThreads()
            )
        );

        NAppHost::TLoop loop;

        const ui16 httpPort = config.GetServer().GetHttpPort();

        // [APPHOSTSUPPORT-358] Crutch in order to '/admin?action=... ' work.
        loop.Add(httpPort, "/never_use_this_route", [](NAppHost::TServiceContextPtr) {
            return NThreading::MakeFuture();
        });

        NPrivate::ConfigureLoop(loop, config.GetServer());

        logFrame->LogEvent(NEvClass::TMatrixStartServicesIntegration());
        TServiceIntegrator<TServices...> services(loop, httpPort, servicesCommonContext);
        logFrame->LogEvent(NEvClass::TMatrixServicesAreIntegrated());

        NPrivate::TAdminHandleListener<TServiceIntegrator<TServices...>> adminHandleListener(httpPort, &services);
        loop.SetAdminHandleListener(&adminHandleListener);

        logFrame->LogEvent(NEvClass::TMatrixServerStarted());
        logFrame->Flush();
        loop.Loop(config.GetServer().GetLoopThreads(), ServiceName);
    } catch (const std::exception& ex) {
        const auto error = FormatCurrentException();

        Cerr << error << Endl;
        logFrame->LogEvent(NEvClass::TMatrixServerStartError(error));

        return false;
    }

    return true;
}

} // NPrivate

template <
    const char* ServiceName,
    typename TServicesCommonContextBuilder,
    typename ...TServices
>
int Run(int argc, const char* argv[]) {
    auto config = NProtoConfig::GetOpt<typename TServicesCommonContextBuilder::TCommonContextType::TConfigType>(argc, argv, "/proto_config/config.json");
    if (!config.GetServer().HasGrpcPort()) {
        config.MutableServer()->SetGrpcPort(config.GetServer().GetHttpPort() + 1);
    }

    if (!NPrivate::RunLoop<ServiceName, TServicesCommonContextBuilder, TServices...>(config)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // namespace NMatrix::NApplication
