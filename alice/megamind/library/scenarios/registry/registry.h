#pragma once

#include <alice/megamind/library/scenarios/helpers/scenario_ref.h>
#include <alice/megamind/library/scenarios/interface/protocol_scenario.h>
#include <alice/megamind/library/scenarios/registry/interface/registry.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice {

class TScenarioRegistry : public IScenarioRegistry {
public:

    void RegisterConfigBasedAppHostProxyProtocolScenario(THolder<TConfigBasedAppHostProxyProtocolScenario> scenario) override {
        ScenarioRefs.push_back(MakeIntrusive<TScenarioRef<TConfigBasedAppHostProxyProtocolScenario>>(*scenario));
        DoRegister(std::move(scenario));
    }

    void RegisterConfigBasedAppHostPureProtocolScenario(THolder<TConfigBasedAppHostPureProtocolScenario> scenario) override {
        ScenarioRefs.push_back(MakeIntrusive<TScenarioRef<TConfigBasedAppHostPureProtocolScenario>>(*scenario));
        DoRegister(std::move(scenario));
    }

    const TScenarioRefs& GetScenarioRefs() const {
        return ScenarioRefs;
    }

    const TFramesToScenarios& GetFramesToScenarios() const override {
        return FramesToScenarios;
    }

private:
    void DoRegister(THolder<TScenario> scenario) {
        Y_ASSERT(scenario);

        const TString name = scenario->GetName();
        const auto frames = scenario->GetAcceptedFrames();
        auto&& [it, inserted] = Scenarios.emplace(name, std::move(scenario));
        Y_ENSURE(inserted, "Duplicate scenario name " << name);

        for (const auto& frame : frames) {
            FramesToScenarios[frame].push_back(name);
        }
    }

private:
    TScenarioRefs ScenarioRefs;
    THashMap<TString, THolder<TScenario>> Scenarios;
    TFramesToScenarios FramesToScenarios;
};

} // namespace NAlice
