#pragma once

#include <alice/bass/libs/scheduler/cache.h>

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/globalctx/globalctx.h>

#include <alice/library/geo/geodb.h>
#include <alice/library/logger/logger.h>

#include <alice/nlg/library/nlg_renderer/fwd.h>

#include <catboost/libs/model/model.h>

#include <kernel/formula_storage/formula_storage.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/logger/priority.h>

#include <logbroker/unified_agent/client/cpp/client.h>

#include <util/thread/pool.h>

#include <memory>

namespace NAlice::NMegamind {

class TGlobalCtx : public IGlobalCtx {
public:
    TGlobalCtx(const TConfig& config, TRTLogClient& rtLogClient, const TScenarioConfigRegistry& registry,
               const TCombinatorConfigRegistry& combinatorRegistry, NMetrics::ISensors& sensors,
               const TClassificationConfig& classificationConfig);

    ~TGlobalCtx();

    const TConfig& Config() const override {
        return Config_;
    }

    const NMegamind::TClassificationConfig& ClassificationConfig() const override {
        return ClassificationConfig_;
    }

    TRTLogger& BaseLogger() override {
        return BaseLogger_;
    }

    TRTLogger RTLogger(const TStringBuf token, const bool session = false,
                       TMaybe<ELogPriority> logPriorityFromRequest = Nothing()) const override {
        return RTLogClient_.CreateLogger(token, session, logPriorityFromRequest);
    }

    TRTLogClient& RTLogClient() const override {
        return RTLogClient_;
    }

    NMetrics::ISensors& ServiceSensors() override {
        return Sensors_;
    }

    const NFactorSlices::TFactorDomain& GetFactorDomain() const override {
        return FactorDomain_;
    }

    const NAlice::TFormulasStorage& GetFormulasStorage() const override {
        return FormulasStorage_;
    }

    TLog& MegamindAnalyticsLog() override {
        return MegamindAnalyticsLog_;
    }

    TLog& MegamindProactivityLog() override {
        return MegamindProactivityLog_;
    }

    const NNlg::INlgRenderer& GetNlgRenderer() const override {
        return *NlgRenderer_;
    }

    const NGeobase::TLookup& GeobaseLookup() const override {
        return GeobaseLookup_;
    }

    const TScenarioConfigRegistry& ScenarioConfigRegistry() const override {
        return ScenarioConfigRegistry_;
    }

    const TCombinatorConfigRegistry& CombinatorConfigRegistry() const override {
        return CombinatorConfigRegistry_;
    }

    const TString& RngSeedSalt() const override {
        return RngSeedSalt_;
    }

    IScheduler& Scheduler() override {
        return Scheduler_;
    }

    const TString& ProxyHost() const override {
        return ProxyHost_;
    }

    ui32 ProxyPort() const override {
        return ProxyPort_;
    }

    const THttpHeaders& ProxyHeaders() const override {
        return ProxyHeaders_;
    }

    IThreadPool& RequestThreads() override {
        return RequestThreads_;
    }

    const TPartialPreCalcer& PartialPreClassificationCalcer() const override {
        return *PartialPreClassificationCalcer_;
    }

private:
    static TConfig MakeConfig(const TConfig& config);

    friend class TGlobalCtxTestSuite;
private:
    const TConfig Config_;
    TClassificationConfig ClassificationConfig_;
    TRTLogClient& RTLogClient_;
    TRTLogger BaseLogger_;
    NMetrics::ISensors& Sensors_;
    NBASS::TCacheManager Scheduler_;
    const NFactorSlices::TFactorDomain FactorDomain_;
    ::TFormulasStorage RawFormulasStorage_;
    TFormulasDescription FormulasDescription_;
    NAlice::TFormulasStorage FormulasStorage_;
    TMaybe<TLog> UnifiedAgentClientLog_;
    NUnifiedAgent::TClientPtr UnifiedAgentClient_;
    TFileLogBackend MegamindAnalyticsLogFile;
    TLog MegamindAnalyticsLog_;
    TFileLogBackend MegamindProactivityLogFile;
    TLog MegamindProactivityLog_;
    NNlg::INlgRendererPtr NlgRenderer_;
    const NGeobase::TLookup GeobaseLookup_;
    const TScenarioConfigRegistry ScenarioConfigRegistry_;
    const TCombinatorConfigRegistry CombinatorConfigRegistry_;
    const TString RngSeedSalt_;
    const TString ProxyHost_;
    const ui32 ProxyPort_;
    const THttpHeaders ProxyHeaders_;
    TThreadPool RequestThreads_;
    std::unique_ptr<TPartialPreCalcer> PartialPreClassificationCalcer_;
};

} // namespace NAlice::NMegamind
