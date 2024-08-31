#include "runner.h"

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/hollywood/library/dispatcher/apphost.h>
#include <alice/hollywood/library/framework/core/scenario_factory.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/registry/hw_service_registry.h>
#include <alice/hollywood/library/registry/secret_registry.h>
#include <alice/hollywood/library/resources/nlg_translations.h>

#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/sensors_queue.h>
#include <alice/library/metrics/sensors_dumper/sensors_dumper.h>
#include <alice/library/metrics/service.h>
#include <alice/library/metrics/util.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/monlib/metrics/metric_registry.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/join.h>
#include <util/system/mlock.h>

#include <signal.h>

namespace NAlice::NHollywood {

namespace {

void SignalHandler(int signal) {
    Cerr << "Signal handler: " << signal << Endl;
    Cerr << "Backtrace is:" << Endl;
    TBackTrace bt;
    bt.Capture();
    Cerr << bt.PrintToString() << Endl;
    exit(signal);
}

// JFYI: Most of this class was consciously copied from alice/megamind/server/main.cpp
class TSensorsImpl final : public NMetrics::ISensors, NNonCopyable::TNonCopyable {
public:
    TSensorsImpl()
        : SolomonSensors_{MakeAtomicShared<NMonitoring::TMetricRegistry>()}
        , SensorsDumper_(*SolomonSensors_)
    {
    }

    void AddRate(NMonitoring::TLabels&& labels, i32 delta) override {
        SolomonSensors_->Rate(std::move(labels))->Add(delta);
    }

    void IncRate(NMonitoring::TLabels&& labels) override {
        AddRate(std::move(labels), 1);
    }

    void AddHistogram(NMonitoring::TLabels&& labels, ui64 value, const TVector<double>& bins) override {
        SolomonSensors_->HistogramRate(std::move(labels), NMonitoring::ExplicitHistogram(bins))->Record(value);
    }

    void AddIntGauge(NMonitoring::TLabels&& labels, i64 value) override {
        SolomonSensors_->IntGauge(std::move(labels))->Add(value);
    }

    void SetIntGauge(NMonitoring::TLabels&& labels, i64 value) override {
        SolomonSensors_->IntGauge(std::move(labels))->Set(value);
    }

    TSensorsDumper& SensorsDumper() {
        return SensorsDumper_;
    }

private:
    TAtomicSharedPtr<NMonitoring::TMetricRegistry> SolomonSensors_;
    TSensorsDumper SensorsDumper_;
};

ELockAllMemoryFlags TransformMemoryLockFlags(const TConfig::ELockMemory option) {
    switch (option) {
        case TConfig::ELockMemory::TConfig_ELockMemory_None:
            return {};
        case TConfig::ELockMemory::TConfig_ELockMemory_Startup:
            return LockCurrentMemory;
        case TConfig::ELockMemory::TConfig_ELockMemory_All:
            return LockCurrentMemory | LockFutureMemory;
    }
}

int Run(int argc, const char** argv) {
    TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv);

    if (config.GetUseSignalFilter()) {
        signal(SIGSEGV, SignalHandler);
    }

    if (config.GetScenarios().empty()) {
        ythrow yexception() << "Missing scenarios list. Please add them via --scenarios param";
    }

    TSensorsImpl sensors;
    TDumpRequestsModeConfig dumpRequestsModeConfig{config.GetDumpRequestsMode().GetEnabled(),
                                                   config.GetDumpRequestsMode().GetOutputDirPath()};
    TGlobalContext globalContext{config, sensors, sensors.SensorsDumper(), dumpRequestsModeConfig};
    globalContext.CommonResources() = LoadCommonResources(globalContext.BaseLogger(), config);

    TSecretRegistry& secretRegistry = TSecretRegistry::Get();
    secretRegistry.CreateSecrets(config.GetFailOnEmptySecrets());

    TSet<TString> scenarios{config.GetScenarios().begin(), config.GetScenarios().end()};
    if (config.ScenariosSize() != scenarios.size()) {
        TSet<TString> scenariosCopy{scenarios};
        TVector<TString> duplicateScenarios;
        for (const auto& scenario : config.GetScenarios()) {
            if (auto it = scenariosCopy.find(scenario); it != scenariosCopy.end()) {
                scenariosCopy.erase(it);
            } else {
                duplicateScenarios.push_back(scenario);
            }
        }
        ythrow yexception() << "Duplicate scenario names in config: " << JoinSeq(", ", duplicateScenarios);
    }
    TScenarioRegistry& registry = TScenarioRegistry::Get();
    registry.CreateScenarios(
        scenarios,
        config.GetScenarioResourcesPath(),
        globalContext.CommonResources().Resource<TNlgTranslationsResource>().GetTranslationsContainer(),
        config.HasIgnoreMissingScenarios());

    // Load old scenarios fastdata
    for (const auto& name : scenarios) {
        globalContext.FastData().Register(registry.GetScenario(name).FastDataInfo());
    }
    // Load new scenarios fastdata
    NHollywoodFw::NPrivate::TScenarioFactory::Instance().RegisterAllFastData(globalContext.FastData());

    THttpProxyRequestBuilder::SetCurrentDc(config.GetCurrentDc());

    THwServiceRegistry& serviceRegistry = THwServiceRegistry::Get();
    serviceRegistry.CreateHandles(config.GetHwServicesConfig(), config.GetHwServicesResourcesPath());
    TApphostDispatcher appHostDispatcher(config.GetAppHostConfig(), scenarios, registry, serviceRegistry, globalContext, config.GetEnableCommonHandles());

    if (config.HasHwfDump()) {
        // Just dump scenario info and exit
        registry.DumpScenarios();
    } else {
        // Perform full HW apphost loop

        // TODO(a-square): if we still have hiccups at major pagefaults, consider splitting Dispatch()
        // into start and wait functions and calling LockAllMemory in between
        auto mlockFlags = TransformMemoryLockFlags(config.GetLockMemory());
        if (mlockFlags) {
            // see config.proto for explanation of what the flags mean
            try {
                LockAllMemory(mlockFlags);
                globalContext.Sensors().SetIntGauge({{"name", "instances_with_non_locked_memory"}}, 0);
            } catch (yexception& e) {
                Cerr << "Failed to lock memory, error: " << e.what() << Endl;
                globalContext.Sensors().SetIntGauge({{"name", "instances_with_non_locked_memory"}}, 1);
            }
        }

        globalContext.FastData().Reload();
        appHostDispatcher.Dispatch();
    }
    registry.ShutdownHandles();
    UnlockAllMemory();
    return EXIT_SUCCESS;
}

} // anonymous namespace

int RunHollywood(int argc, const char** argv) {
    try {
        return Run(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return EXIT_FAILURE;
    }
}

} // namespace NAlice::NHollywood
