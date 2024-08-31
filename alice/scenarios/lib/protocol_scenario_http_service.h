#pragma once
#include "http_service.h"
#include "protocol_scenario.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

class TProtocolScenarioHttpService : public TProtoBasedHttpService {
public:
    static constexpr const char RUN_SUFFIX[] = "run";
    static constexpr const char APPLY_SUFFIX[] = "apply";
    static constexpr const char COMMIT_SUFFIX[] = "commit";

    using TProtoBasedHttpService::TProtoBasedHttpService;
    using TScenarioRunRequest = NScenarios::TScenarioRunRequest;
    using TScenarioApplyRequest = NScenarios::TScenarioApplyRequest;

    void RegisterScenario(const TString& scenarioPath, THolder<const IProtocolScenario> scenario) {
        Y_ASSERT(scenario);

        Scenarios.emplace_back(std::move(scenario));
        const IProtocolScenario* scenarioPtr = Scenarios.back().Get();

        RegisterHandler<TScenarioRunRequest, NScenarios::TScenarioRunResponse>(
            scenarioPath + RUN_SUFFIX,
            [scenarioPtr](const TScenarioRunRequest& request, TRTLogger& logger) {
                return scenarioPtr->Run(request, logger);
            }
        );

        RegisterHandler<TScenarioApplyRequest, NScenarios::TScenarioCommitResponse>(
            scenarioPath + COMMIT_SUFFIX,
            [scenarioPtr](const TScenarioApplyRequest& request, TRTLogger& logger) {
                return scenarioPtr->Commit(request, logger);
            }
        );

        RegisterHandler<TScenarioApplyRequest, NScenarios::TScenarioApplyResponse>(
            scenarioPath + APPLY_SUFFIX,
            [scenarioPtr](const TScenarioApplyRequest& request, TRTLogger& logger) {
                return scenarioPtr->Apply(request, logger);
            }
        );
    }

private:
    TVector<THolder<const IProtocolScenario>> Scenarios;
};

} // namespace NAlice
