#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>

#include <functional>
#include <memory>

namespace NAlice::NHollywood {

// NOTE(a-square): handles might not be cheap to move, but TScenario objects are
using TScenarioProducer = std::function<TScenario()>;

class TScenarioRegistry {
public:
    static TScenarioRegistry& Get();

public:
    void AddScenarioProducer(const TString& name, const TScenarioProducer& scenario);
    void CreateScenarios(TSet<TString>& scenarioNames, const TFsPath& resourcesBasePath, const NNlg::ITranslationsContainerPtr& nlgTranslations, bool ignoreMissingScenarios);
    const TScenario& GetScenario(TStringBuf path) const;
    void DumpScenarios() const;
    void ShutdownHandles() const;

private:
    THashMap<TString, TScenarioProducer> ScenarioProducers_;
    THashMap<TString, std::unique_ptr<TScenario>> Scenarios_;
    TSet<TString> MissedScenarios_;
};

struct TScenarioRegistrator {
    TScenarioRegistrator(const TString& name, TScenarioProducer&& scenario);
};

} // namespace NAlice::NHollywood

#define REGISTER_SCENARIO(name, ...) \
static NAlice::NHollywood::TScenarioRegistrator \
Y_GENERATE_UNIQUE_ID(ScenarioRegistrator)(name, []() -> TScenario { return TScenario{name}.__VA_ARGS__; })
