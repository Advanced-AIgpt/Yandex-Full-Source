#include "watchdog.h"

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/dispatcher/apphost_dispatcher.h>
#include <alice/megamind/library/globalctx/impl/globalctx.h>
#include <alice/megamind/library/handlers/registry/construct.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/util/config.h>

#include <alice/library/geo/geodb.h>
#include <alice/library/logger/logger.h>
#include <alice/library/metrics/names.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/sensors_queue.h>
#include <alice/library/metrics/service.h>
#include <alice/library/metrics/util.h>
#include <alice/library/typed_frame/typed_frames.h>

#include <alice/megamind/protos/common/required_messages/required_messages.pb.h>

#include <infra/udp_click_metrics/client/client.h>

#include <kernel/factor_slices/factor_domain.h>
#include <kernel/formula_storage/formula_storage.h>

#include <library/cpp/logger/thread.h>
#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/service/pages/registry_mon_page.h>
#include <library/cpp/monlib/service/pages/version_mon_page.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <memory>
#include <apphost/api/service/cpp/service.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/scope.h>
#include <util/generic/yexception.h>
#include <util/random/random.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/env.h>

#include <signal.h>

using namespace NAlice;

namespace {

class TSensorsImpl final : public NMetrics::ISensors, NNonCopyable::TNonCopyable {
public:
    using TMonService = NMonitoring::TMonService2;
    using TMonServicePtr = THolder<TMonService>;

public:
    TSensorsImpl(
        const TConfig& config,
        TMaybe<NUdpClickMetrics::TSelfBalancingClient>& UdpClient
    )
        : SolomonSensors_{MakeAtomicShared<NMonitoring::TMetricRegistry>()}
        , SensorsQueue_(config.HasFileSensors() ? MakeHolder<NAlice::NMetrics::TSensorsQueue>() : nullptr)
        , UdpClient_(UdpClient)
    {
    }

    void AddRate(NMonitoring::TLabels&& labels, i32 delta) override {
        if (auto udpType = labels.Get(NSignal::UDP_SENSOR)) {
            if (UdpClient_.Defined()) {
                NUdpClickMetrics::NApi::TReqIncreaseClickMetrics request;
                if (udpType.value()->Value() == NSignal::IS_WINNER_SCENARIO) {
                    auto* mmIntents = request.MutableMegamindIntentMetrics();
                    mmIntents->SetIntent(TString{labels.Get(NSignal::INTENT).value()->Value()});
                    mmIntents->SetScenarioName(TString{labels.Get(NSignal::SCENARIO_NAME).value()->Value()});
                    mmIntents->SetIsWinnerScenario(TString{labels.Get(NSignal::IS_WINNER_SCENARIO).value()->Value()});
                } else if (udpType.value()->Value() == NSignal::IS_IRRELEVANT) {
                    auto* mmIrrelevant = request.MutableMegamindIrrelevantMetrics();
                    mmIrrelevant->SetIntentName(TString{labels.Get(NSignal::INTENT).value()->Value()});
                    mmIrrelevant->SetScenarioName(TString{labels.Get(NSignal::SCENARIO_NAME).value()->Value()});
                } else {
                    return;
                }
                UdpClient_->IncreaseMetrics(request);
            }
            return;
        }
        if (SensorsQueue()) {
            SensorsQueue()->Add({labels, delta, TInstant::Now()});
        }

        SolomonSensors_->Rate(std::move(labels))->Add(delta);
    }

    void IncRate(NMonitoring::TLabels&& labels) override {
        AddRate(std::move(labels), 1);
    }

    void AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins) override {
        if (SensorsQueue()) {
            SensorsQueue()->Add({labels, static_cast<i64>(value), TInstant::Now()});
        }

        SolomonSensors_->HistogramRate(std::move(labels), NMonitoring::ExplicitHistogram(bins))->Record(value);
    }

    void AddIntGauge(NMonitoring::TLabels&& labels, i64 value) override {
        if (SensorsQueue()) {
            SensorsQueue()->Add({labels, value, TInstant::Now()});
        }

        SolomonSensors_->IntGauge(std::move(labels))->Add(value);
    }

    void SetIntGauge(NMonitoring::TLabels&& labels, i64 value) override {
        if (SensorsQueue()) {
            SensorsQueue()->Add({labels, value, TInstant::Now()});
        }

        SolomonSensors_->IntGauge(std::move(labels))->Set(value);
    }

    NMetrics::TSensorsQueue* SensorsQueue() {
        return SensorsQueue_.Get();
    }

    void RegisterService(TMonService& service) {
        service.Register(new NMonitoring::TVersionMonPage);
        service.Register(new NMetrics::TGolovanCountersPage("stat", *SolomonSensors_));
        service.Register(new NMonitoring::TMetricRegistryPage("counters", "Counters", SolomonSensors_));
    }

    static TMonServicePtr ConstructAndRunMonService(IGlobalCtx& globalCtx, TSensorsImpl& sensors) {
        const TConfig& config = globalCtx.Config();
        if (!config.HasMonService()) {
            return {};
        }

        auto monService = MakeHolder<TSensorsImpl::TMonService>(MonitoringPort(globalCtx.Config()));
        sensors.RegisterService(*monService);
        return monService;
    }

private:
    TAtomicSharedPtr<NMonitoring::TMetricRegistry> SolomonSensors_;
    THolder<NMetrics::TSensorsQueue> SensorsQueue_;
    TMaybe<NUdpClickMetrics::TSelfBalancingClient>& UdpClient_;
};

void FlushSensors(const TString& filename, NAlice::NMetrics::TSensorsQueue& sensorsQueue) {
    TFileOutput out(TFile(filename, OpenAlways | WrOnly | ForAppend));
    for (const auto& metric : sensorsQueue.StealMetrics()) {
        NJsonWriter::TBuf buf;
        buf.BeginObject();

        buf.WriteKey("labels");
        buf.BeginObject();
        for (const auto& label : metric.Labels) {
            buf.WriteKey(label.Name());
            buf.WriteString(label.Value());
        }
        buf.EndObject();

        buf.WriteKey("value");
        buf.WriteLongLong(metric.Value);

        buf.WriteKey("time");
        buf.WriteString(metric.Time.ToString());
        buf.EndObject();
        out << buf.Str() << "\n";
    }
}

bool IsWatchDogEnabled(const TConfig& config) {
    return config.GetWatchDogRequestTimeoutSeconds();
}

std::unique_ptr<NMegamind::TAppHostDispatcher> ConstructAppHost(IGlobalCtx& globalCtx) {
    if (!globalCtx.Config().HasAppHost()) {
        return {};
    }
    return std::make_unique<NMegamind::TAppHostDispatcher>(globalCtx);
}

template <typename TStartCallback>
bool StartService(TStartCallback&& start, TAtomic& isShutdownInitiated) {
    bool isStarted = false;
    for (ui32 attempt = 0, maxAttemts = 10;
         attempt < maxAttemts && !(isStarted = start()) && !AtomicGet(isShutdownInitiated);
         ++attempt
    ) {
        Sleep(TDuration::MilliSeconds(500));
    }
    return isStarted;
}

void LogProtocolScenarios(TRTLogger& logger, const TScenarioConfigRegistry& registry) {
    const auto& scenarios = registry.GetScenarioConfigs();
    TStringBuilder builder{};
    builder << Endl << "Loaded " << scenarios.size() << " protocol scenarios:" << Endl;
    for (const auto& [name, config] : scenarios) {
        builder << " - '" << name << "': ";
        builder << '[' << (config.GetEnabled() ? "enabled" : "disabled") << ']';
        builder << '[' << config.GetHandlers().GetBaseUrl() << ']';
        builder << '[' << ERequestType_Name(config.GetHandlers().GetRequestType()) << ']';
        builder << Endl;
    }
    LOG_INFO(logger) << builder;
}

void LogCombinators(TRTLogger& logger, const TCombinatorConfigRegistry& registry) {
    const auto& combinators = registry.GetCombinatorConfigs();
    TStringBuilder builder{};
    builder << Endl << "Loaded " << combinators.size() << " combinators:" << Endl;
    for (const auto& [name, config] : combinators) {
        builder << " - '" << name << "': ";
        builder << '[' << (config.GetEnabled() ? "enabled" : "disabled") << ']';
        builder << Endl;
    }
    LOG_INFO(logger) << builder;
}

void InitAndPopulateRegistry(IGlobalCtx& globalCtx,
                             NMegamind::TAppHostDispatcher* appHostDispatcher,
                             TMaybe<NInfra::TLogger>& udpLogger,
                             TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient)
{
    std::unique_ptr<TRegistry> registry;
    const auto& config = globalCtx.Config();
    if (IsWatchDogEnabled(config)) {
        registry = std::make_unique<NMegamind::TWatchDogRegistry>(appHostDispatcher, globalCtx);
    } else {
        registry = std::make_unique<TRegistry>(appHostDispatcher);
    }

    PopulateRegistry(globalCtx, udpLogger, udpClient, *registry);
}

int Main(int argc, const char** argv) {
    const TConfig config{LoadConfig(argc, argv)};
    TRTLogClient rtLogClient{config.GetRTLog()};

    auto log = rtLogClient.CreateLogger({} /* token */, false /* session */);

    TScenarioConfigRegistry registry{};
    for (const auto& [_, config] :
        LoadScenarioConfigs(config.GetScenarioConfigsPath(), /* validateConfigs= */ true)) {
        registry.AddScenarioConfig(config);
    }
    TCombinatorConfigRegistry combinatorRegistry{};
    for (const auto& [_, config] :
        LoadCombinatorConfigs(config.GetCombinatorConfigsPath())) {
        combinatorRegistry.AddCombinatorConfig(TCombinatorConfig{config});
    }
    const auto classificationConfig = LoadClassificationConfig(config.GetClassificationConfigPath());

    NAlice::ValidateTypedSemanticFrames();

    // all possible google.protobuf.Any values should be linked inside MM while jsonToProto conversion exists
    const NAlice::NMegamind::TRequiredMessages requiredProtoMessages;
    Y_UNUSED(requiredProtoMessages);

    LogProtocolScenarios(log, registry);
    LogCombinators(log, combinatorRegistry);

    TMaybe<NInfra::TLogger> udpLogger = Nothing();
    TMaybe<NUdpClickMetrics::TSelfBalancingClient> udpClient = Nothing();
    if (config.GetEnableUdpMetrics() && config.HasEventLogConfig() && config.HasStatisticsClientConfig()) {
        udpLogger.ConstructInPlace(config.GetEventLogConfig());
        udpClient.ConstructInPlace(config.GetStatisticsClientConfig(), udpLogger.GetRef());
    }

    TSensorsImpl sensors(config, udpClient);

    NMegamind::TGlobalCtx globalCtx{config, rtLogClient, registry, combinatorRegistry, sensors, classificationConfig};

    TSensorsImpl::TMonServicePtr monService = TSensorsImpl::ConstructAndRunMonService(globalCtx, sensors);

    auto appHostDispatcher = ConstructAppHost(globalCtx);

    TAtomic isShutdownInitiated{false};
    auto shutdown = [&log, &monService, &isShutdownInitiated](int) {
        if (!AtomicCas(&isShutdownInitiated, /* desired= */ true, /* expected= */ false)) {
            // Skip duplicate shutdown calls.
            return;
        }

        if (monService) {
            LOG_INFO(log) << "Monitoring service is shutting down";
            monService->Shutdown();
            LOG_INFO(log) << "Monitoring service has been stopped";
        }
    };

    InitAndPopulateRegistry(globalCtx, appHostDispatcher.get(), udpLogger, udpClient);

    if (sensors.SensorsQueue()) {
        globalCtx.Scheduler().Schedule([&sensors, &config]() {
            FlushSensors(config.GetFileSensors(), *sensors.SensorsQueue());
            return TDuration::Seconds(3);
        });
    }
    auto rotate = [&rtLogClient](int /* signal */) {
        rtLogClient.Rotate();
    };

    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);
    SetAsyncSignalFunction(SIGHUP, rotate);

    {
        if (monService) {
            auto onMonServiceStart = [&monService, &log]() -> bool {
                if (monService->Start()) {
                    return true;
                }
                LOG_INFO(log) << "Starting monitoring HTTP server..." << Endl;
                return false;
            };
            if (!StartService(onMonServiceStart, isShutdownInitiated)) {
                LOG_CRIT(log) << "Unable to start monitoring service";
                return EXIT_FAILURE;
            }
            LOG_INFO(log) << "Monitoring started on port " << MonitoringPort(globalCtx.Config()) << Endl;
        }

        if (appHostDispatcher) {
            if (!AtomicGet(isShutdownInitiated)) {
                appHostDispatcher->Start();
            } else {
                LOG_CRIT(log) << "Unable to start apphost loop";
                return EXIT_FAILURE;
            }
        }
        Y_SCOPE_EXIT(&appHostDispatcher, &log) {
            if (appHostDispatcher) {
                LOG_INFO(log) << "Apphost loop is shutting down" << Endl;
                appHostDispatcher->Stop();
                LOG_INFO(log) << "Apphost loop has been stopped" << Endl;
            }
        };

        if (monService) {
            monService->Wait();
        }
    }

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char** argv) {
    try {
        return Main(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return EXIT_FAILURE;
    }
}
