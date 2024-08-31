#pragma once

#include "defs.h"

#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/hollywood/library/hw_service_context/context.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NCombinators {

class TCombinatorContextWrapper {
public:
    TCombinatorContextWrapper(
        THwServiceContext& Ctx,
        const TCombinatorRequestWrapper& Request,
        NScenarios::TScenarioRunResponse& ResponseForRenderer);
    THwServiceContext& Ctx();
    const TCombinatorRequestWrapper& Request();
    NScenarios::TScenarioRunResponse& ResponseForRenderer();
    TRTLogger& Logger();

private:
    THwServiceContext& Ctx_;
    const TCombinatorRequestWrapper& Request_;
    NScenarios::TScenarioRunResponse& ResponseForRenderer_;
    TRTLogger& Logger_;
};

}
