#include "globalctx.h"

#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/scenarios/registry/registry.h>
#include <alice/megamind/nlg/register.h>

#include <alice/library/metrics/sensors.h>
#include <alice/library/util/rng.h>

#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>

#include <logbroker/unified_agent/client/cpp/logger/backend.h>

#include <util/stream/file.h>

namespace NAlice::NMegamind {

namespace {

NGeobase::TLookup ConstructGeobaseLookup(const TString& geobasePath, bool lockGeobase) {
    Y_ENSURE(TFsPath(geobasePath).Exists(), "File " << geobasePath << " not found");
    // Default value for LockMemory is false
    return NGeobase::TLookup(geobasePath, NGeobase::TInitTraits{}.LockMemory(lockGeobase));
}

void LoadCatboostModel(const TString& path, TFullModel& model) {
    Y_ENSURE(TFsPath(path).Exists(), "File " << path << " not found");
    TFileInput input(path);
    model.Load(&input);
}

std::unique_ptr<TPartialPreCalcer> LoadPartialPreClassifierModel(const TString& path) {
    TFullModel partialPreModel;
    LoadCatboostModel(path, partialPreModel);
    auto calcer = std::make_unique<TPartialPreCalcer>(std::move(partialPreModel));
    Y_ENSURE(!calcer->GetSlices().empty(), "No slices info in partials preclassification model");
    return calcer;
}

TString ConstructProxyHost(const TConfig& config) {
    if (config.HasViaProxy()) {
        return config.GetViaProxy().GetHost();
    }
    return {};
}

ui32 ConstructProxyPort(const TConfig& config) {
    if (config.HasViaProxy()) {
        return config.GetViaProxy().GetPort();
    }
    return 0;
}

THttpHeaders ConstructProxyHeaders(const TConfig& config) {
    THttpHeaders headers;
    if (config.HasViaProxy()) {
        for (const TString& header : config.GetViaProxy().GetHeaders()) {
            TStringBuf name, value;
            if (TStringBuf(header).TrySplit(':', name, value)) {
                headers.AddHeader(TString{name}, value);
            }
        }
    }
    return headers;
}

TMaybe<TLog> CreateUnifiedAgentClientLog(const TString& unifiedAgentLogFile) {
    if (unifiedAgentLogFile.empty()) {
        return {};
    }
    return CreateLogBackend(unifiedAgentLogFile, ELogPriority::TLOG_INFO, true);
}

NUnifiedAgent::TClientPtr MakeUnifiedAgentClientPtr(const TString& unifiedAgentUri, TMaybe<TLog>& unifiedAgentClientLog) {
    if (unifiedAgentUri.empty()) {
        return nullptr;
    }

    NUnifiedAgent::TClientParameters unifiedAgentParams{unifiedAgentUri};
    if (unifiedAgentClientLog.Defined()) {
        unifiedAgentClientLog->SetDefaultPriority(ELogPriority::TLOG_INFO);
        unifiedAgentParams.SetLog(*unifiedAgentClientLog);
    }

    return NUnifiedAgent::MakeClient(unifiedAgentParams);
}

TLog CreateLogBackend(NUnifiedAgent::TClientPtr unifiedAgentClient, const TString& unifiedAgentLogType, TFileLogBackend* fileLogBackendPtr) {
    if (unifiedAgentClient != nullptr && !unifiedAgentLogType.empty()) {
        THashMap<TString, TString> sessionMetaFlags;
        sessionMetaFlags["log_type"] = unifiedAgentLogType;

        NUnifiedAgent::TSessionParameters sessionParametrs{};
        sessionParametrs.SetMeta(sessionMetaFlags);
        return TLog{NUnifiedAgent::AsLogBackend(unifiedAgentClient->CreateSession(sessionParametrs))};
    }
    return TLog{MakeHolder<TThreadedLogBackend>(fileLogBackendPtr)};
}

} // namespace

TConfig TGlobalCtx::MakeConfig(const TConfig& config) {
    TConfig result{config};

    const auto& defaultScenarioConfig = config.GetScenarios().GetDefaultConfig();
    auto& scenarioConfigs = *result.MutableScenarios()->MutableConfigs();
    for (const auto& [name, originalScenarioConfig]: config.GetScenarios().GetConfigs()) {
        auto& scenarioConfig = scenarioConfigs[name];
        scenarioConfig.CopyFrom(defaultScenarioConfig);
        scenarioConfig.MergeFrom(originalScenarioConfig);
    }

    return result;
}

TGlobalCtx::TGlobalCtx(const TConfig& config, TRTLogClient& rtLogClient, const TScenarioConfigRegistry& registry,
                       const TCombinatorConfigRegistry& combinatorRegistry, NMetrics::ISensors& sensors,
                       const TClassificationConfig& classificationConfig)
    : Config_{MakeConfig(config)}
    , ClassificationConfig_{classificationConfig}
    , RTLogClient_{rtLogClient}
    , BaseLogger_{RTLogClient_.CreateLogger({}, false)}
    , Sensors_{sensors}
    , FactorDomain_{CreateFactorDomain()}
    , FormulasDescription_{ClassificationConfig().GetScenarioClassificationConfigs(), Config().GetFormulasPath(), BaseLogger()}
    , FormulasStorage_{RawFormulasStorage_, FormulasDescription_}
    , UnifiedAgentClientLog_{CreateUnifiedAgentClientLog(config.GetUnifiedAgentLogFile())}
    , UnifiedAgentClient_{MakeUnifiedAgentClientPtr(config.GetUnifiedAgentUri(), UnifiedAgentClientLog_)}
    , MegamindAnalyticsLogFile{config.GetMegamindAnalyticsLogFile()}
    , MegamindAnalyticsLog_{CreateLogBackend(UnifiedAgentClient_, "megamind_analytics_log", &MegamindAnalyticsLogFile)}
    , MegamindProactivityLogFile{config.GetMegamindProactivityLogFile()}
    , MegamindProactivityLog_{CreateLogBackend(UnifiedAgentClient_, "megamind_proactivity_log", &MegamindProactivityLogFile)}
    , GeobaseLookup_{ConstructGeobaseLookup(Config_.GetGeobasePath(), Config_.GetLockGeobase())}
    , ScenarioConfigRegistry_{registry}
    , CombinatorConfigRegistry_{combinatorRegistry}
    , RngSeedSalt_(Config_.GetRandomSeedSalt())
    , ProxyHost_(ConstructProxyHost(Config_))
    , ProxyPort_(ConstructProxyPort(Config_))
    , ProxyHeaders_(ConstructProxyHeaders(Config_))
{
    RawFormulasStorage_.AddFormulasFromDirectoryRecursive(Config_.GetFormulasPath());
    {
        LOG_INFO(BaseLogger()) << "Successfully loaded models from " << Config_.GetFormulasPath().Quote() << ":";
        TVector<TString> names;
        RawFormulasStorage_.GetDefinedFormulaNames(names, /* sorted */ true);
        for (const TString& name : names) {
            LOG_INFO(BaseLogger()) << "  " << name;
        }
    }

    PartialPreClassificationCalcer_ = LoadPartialPreClassifierModel(config.GetPartialPreClassificationModelPath());

    MegamindAnalyticsLog_.ReopenLog();
    MegamindProactivityLog_.ReopenLog();

    // XXX(a-square): the seed needs to be fixed so that global state of NLG templates
    // is reproducible, this generator is *not* used for subsequent phrase/card rendering
    TRng rng{4};  // https://xkcd.com/221/
    NlgRenderer_ = ::NAlice::NNlg::CreateNlgRendererFromRegisterFunction(::NAlice::NMegamind::NNlg::RegisterAll, rng);

    RequestThreads_.Start(Config_.GetRequestThreadsCount());
}

TGlobalCtx::~TGlobalCtx() = default;

} // namespace NAlice::NMegamind
