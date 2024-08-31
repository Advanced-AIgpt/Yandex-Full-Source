#pragma once

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/logger/logger.h>

namespace NAlice {

class IProtocolScenario {
public:
    virtual ~IProtocolScenario() = default;
    virtual NScenarios::TScenarioRunResponse Run(const NScenarios::TScenarioRunRequest& request, TRTLogger& logger) const = 0;
    virtual NScenarios::TScenarioCommitResponse Commit(const NScenarios::TScenarioApplyRequest& request, TRTLogger& logger) const = 0;
    virtual NScenarios::TScenarioApplyResponse Apply(const NScenarios::TScenarioApplyRequest& request, TRTLogger& logger) const = 0;
};

} // namespace NAlice
