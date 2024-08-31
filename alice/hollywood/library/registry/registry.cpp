#include "registry.h"
#include <alice/hollywood/library/framework/core/scenario_factory.h>

#include <util/system/defaults.h>

namespace NAlice::NHollywood {

namespace {

void LoadScenarioResources(TScenario& scenario, const TFsPath& resourcesBasePath) {
    if (auto* resources = scenario.Resources()) {
        Cerr << "Loading resources for scenario " << scenario.Name() << Endl;;
        resources->LoadFromPath(resourcesBasePath / scenario.Name()); // TODO(a-square): load in parallel?
    }
}

void InitNlg(TScenario& scenario, const NNlg::ITranslationsContainerPtr& nlgTranslations) {
    if (auto nlgRegisterFunction = scenario.GetNlgRegistration()) {
        auto rng = TRng(4);
        auto nlg = TCompiledNlgComponent(rng, nlgTranslations, std::move(nlgRegisterFunction));
        scenario.SetNlg(std::move(nlg));
    }
}

} // namespace

TScenarioRegistry& TScenarioRegistry::Get() {
    return *Singleton<TScenarioRegistry>();
};

void TScenarioRegistry::AddScenarioProducer(const TString& name, const TScenarioProducer& scenarioProducer) {
    // NOTE(a-square): at this point we don't have RTLog so this is the only way to log things
    Cerr << "Registering scenario " << name << Endl;
    const auto [_, inserted] = ScenarioProducers_.insert({name, scenarioProducer});
    Y_ENSURE(inserted, "Duplicate scenario name: " << name);
}

void TScenarioRegistry::CreateScenarios(TSet<TString>& scenarioNames, const TFsPath& resourcesBasePath, const NNlg::ITranslationsContainerPtr& nlgTranslations, bool ignoreMissingScenarios) {
    int newScenarioCount = 0;

    NHollywoodFw::NPrivate::TScenarioFactory::Instance().CreateAllScenarios();
    for (const auto& name : scenarioNames) {
        const bool bNewScenarioExist = NHollywoodFw::NPrivate::TScenarioFactory::Instance().FindScenarioName(name) ? true : false;
        if (const auto* scenarioProducer = ScenarioProducers_.FindPtr(name); scenarioProducer != nullptr) {
            auto scenario = (*scenarioProducer)();
            Y_ENSURE(scenario.Name() == name, "Inconsistent scenario name: " << scenario.Name() << " != " << name);
            LoadScenarioResources(scenario, resourcesBasePath);
            InitNlg(scenario, nlgTranslations);
            Scenarios_[name] = std::make_unique<TScenario>(std::move(scenario));
        } else if (bNewScenarioExist) {
            Cout << "Scenario " << name << " was registered with new scenario flow" << Endl;
            newScenarioCount++;
            MissedScenarios_.insert(name); // New scenarios will be removed from scenarioNames to avoid conflicts with old flow
        } else if (ignoreMissingScenarios) {
            Cerr << "Scenario " << name << " is not found in compiled app, skip it" << Endl;
            MissedScenarios_.insert(name);
        } else {
            ythrow yexception() << "Can't find scenario '" << name << "'. "
                               << "Most likely you have a scenario in your config, that this binary doesn't know about";
        }
    }
    if (Scenarios_.size() == 0 && newScenarioCount == 0) {
        ythrow yexception() << "Can not find any scenario in list. Please check scenario resource path and missing scenario options";
    }

    // Remove missed scenarios and scenarios from new flow from source list
    for (const auto& name : MissedScenarios_) {
        scenarioNames.erase(scenarioNames.find(name));
    }
    NHollywoodFw::NPrivate::TScenarioFactory::Instance().FinishInitialization(nlgTranslations, resourcesBasePath);
}

/*
    Dump information about all scenarios in JSON format to stdout
*/
void TScenarioRegistry::DumpScenarios() const {
    TSet<TString> oldScenarios;
    for (const auto& it : Scenarios_) {
        oldScenarios.insert(it.first);
    }
    const auto out = NHollywoodFw::NPrivate::TScenarioFactory::Instance().DumpScenarios(oldScenarios, MissedScenarios_);
    Cout << "BEGIN DUMP" << Endl;
    Cout << out << Endl;
    Cout << "END DUMP" << Endl;
}

const TScenario& TScenarioRegistry::GetScenario(TStringBuf name) const {
    return *Scenarios_.at(name);
}

void TScenarioRegistry::ShutdownHandles() const {
    for (auto& [_, scenario] : Scenarios_) {
        Cerr << "Shutting down scenario " << scenario->Name() << Endl;
        for (auto& handle : scenario->Handles()) {
            handle->Shutdown();
        }
    }
    NHollywoodFw::NPrivate::TScenarioFactory::Instance().DestroyAllScenarios();
}

TScenarioRegistrator::TScenarioRegistrator(const TString& name, TScenarioProducer&& scenarioProducer) {
    TScenarioRegistry::Get().AddScenarioProducer(name, std::move(scenarioProducer));
}

} // namespace NAlice::NHollywood
