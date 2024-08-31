#include <alice/cachalot/library/cachalot.h>

#include <alice/cachalot/library/config/load.h>
#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/activation/service.h>
#include <alice/cachalot/library/modules/cache/service.h>
#include <alice/cachalot/library/modules/gdpr/service.h>
#include <alice/cachalot/library/modules/megamind_session/service.h>
#include <alice/cachalot/library/modules/stats/service.h>
#include <alice/cachalot/library/modules/takeout/service.h>
#include <alice/cachalot/library/modules/vins_context/service.h>
#include <alice/cachalot/library/modules/yabio_context/service.h>
#include <alice/cachalot/library/service.h>

#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <alice/cuttlefish/library/apphost/admin_handle_listener.h>
#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/metrics/metrics.h>
#include <alice/cuttlefish/library/mlock/mlock.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <voicetech/library/itags/itags.h>

#include <apphost/api/service/cpp/service.h>
#include <apphost/api/service/cpp/service_loop.h>

#include <library/cpp/proto_config/load.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/http2.h>
#include <library/cpp/neh/tcp2.h>


using namespace NCachalot;

using TServices = TServiceIntegrator<
    TActivationService,
    TCacheService,
    TMegamindSessionService,
    TStatsService,
    TYabioContextService,
    TGDPRService,
    TTakeoutService,
    TVinsContextService
>;


class TAdminHandleListener : public NAlice::NCuttlefish::TAdminHandleListener {
public:
    using TBase = NAlice::NCuttlefish::TAdminHandleListener;

    explicit TAdminHandleListener(ui16 port, TServices* services)
        : NAlice::NCuttlefish::TAdminHandleListener(port)
        , Services(services)
    {
    }

    void OnShutdown() override {
        Y_ASSERT(Services != nullptr);
        Services->Suspend();
    }

private:
    TServices* Services = nullptr;
};


void InitSolomon() {
    NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();

    metrics.SetBackend(
        NVoice::NMetrics::EMetricsBackend::Solomon,
        MakeHolder<NVoice::NMetrics::TSolomonBackend>(
            NVoice::NMetrics::TAggregationRules(),
            NVoice::NMetrics::MakeMillisBuckets(),
            "cachalot",
            /* maskHost = */ false
        )
    );
}


NAppHost::TLoop& ConfigureLoop(NAppHost::TLoop& loop, const NCachalot::TApplicationSettings& settings, NAlice::NCuttlefish::TLogFramePtr& frame);

void ConfigureLogger(const NCachalot::TApplicationSettings& settings);


inline NAlice::NCuttlefish::TLogFramePtr SpawnFrame(bool needAlwaysSafeAdd = false) {
    return NAlice::NCuttlefish::SpawnLogFrame(needAlwaysSafeAdd);
}


bool RunLoop(const NCachalot::TApplicationSettings& settings) {
    ConfigureLogger(settings);

    // WARNING: It is important to init metrics before lock memory
    NCachalot::TMetrics::GetInstance();
    InitSolomon();

    auto logFrame = SpawnFrame();

    logFrame->LogEvent(NEvClass::InfoMessage(TStringBuilder()
        << "InstanceTags: {"
        "Geo: " << TInstanceTags::Get().Geo << ", "
        "Dc: " << TInstanceTags::Get().Dc << ", "
        "Prj: " << TInstanceTags::Get().Prj << ", "
        "Itype: " << TInstanceTags::Get().Itype << ", "
        "Ctype: " << TInstanceTags::Get().Ctype << '}'
    ));

    try {
        const uint16_t port = settings.Server().Port();

        logFrame->LogEvent(NEvClass::ServerStarting(port));
        NAppHost::TLoop loop;

        ConfigureLoop(loop, settings, logFrame);

        logFrame->LogEvent(NEvClass::InfoMessage("Integrating services..."));
        TServices services(loop, port, settings);

        TAdminHandleListener adminHandleListener(port, &services);
        loop.SetAdminHandleListener(&adminHandleListener);

        NVoice::TryMlockAndReport(settings.LockMemory(), logFrame);

        logFrame->LogEvent(NEvClass::InfoMessage("Starting io loop..."));
        logFrame->Flush();
        loop.Loop(settings.Server().Threads(), "cachalot");
    } catch (const yexception& ex) {
        Cerr << "YException: " << ex.what() << '\n' << ex.BackTrace()->PrintToString() << Endl;
        logFrame->LogEvent(NEvClass::ServerError(ex.what()));
        return 1;
    } catch (const std::exception& ex) {
        Cerr << "Exception: " << ex.what() << Endl;
        logFrame->LogEvent(NEvClass::ServerError(ex.what()));
        return 1;
    }
    return 0;
}


NAppHost::TLoop& ConfigureLoop(NAppHost::TLoop& loop, const NCachalot::TApplicationSettings& settings, NAlice::NCuttlefish::TLogFramePtr& frame) {
    if (settings.Server().HasGrpcPort()) {
        frame->LogEvent(NEvClass::CachalotInfo("GRPC Port", settings.Server().GrpcPort(), 0));
        loop.EnableGrpc(settings.Server().GrpcPort(), true, TDuration::Seconds(3));

        frame->LogEvent(NEvClass::CachalotInfo("GRPC Threads", settings.Server().GrpcThreads(), 0));
        loop.SetGrpcThreadCount(settings.Server().GrpcThreads());
    }

    frame->LogEvent(NEvClass::CachalotInfo("Tools Threads", 1, 0));
    loop.SetToolsThreadCount(1);

    frame->LogEvent(NEvClass::CachalotInfo("Admin Threads", settings.Server().AdminThreads(), 0));
    loop.SetAdminThreadCount(settings.Server().AdminThreads());

    return loop;
}


void ConfigureLogger(const NCachalot::TApplicationSettings& settings) {
    {
        NAlice::NCuttlefish::TLogger::TConfig config;
        config.Filename = settings.Log().Filename();
        NAlice::NCuttlefish::GetLogger().Init(config);
    }
    {
        TString filename = settings.Log().RtLogFilename();
        if (filename) {
            NAliceServiceConfig::TRtLog config;
            config.set_file(filename);
            config.set_service("cachalot");
            try {
                NAlice::NCuttlefish::TRtLogClient::Instance().Init(config);
            } catch (...) {
                TStringStream ss;
                ss << "fail create NRTLog::TClient for filename=\"" << config.file()
                   << "\": " << CurrentExceptionMessage();
                SpawnFrame()->LogEvent(NEvClass::ErrorMessage(ss.Str()));
            }
        }
    }
}


namespace NCachalot::NApplication {

    int Run(int argc, const char **argv, const TString& defaultConfigResource) {
        TApplicationSettings appSettings = NConfig::LoadApplicationConfig(argc, argv, defaultConfigResource);

        if (!RunLoop(appSettings)) {
            return 2;
        }

        return 0;
    }

}   // namespace NCachalot::NApplication
