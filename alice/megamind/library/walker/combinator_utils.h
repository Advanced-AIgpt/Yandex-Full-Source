#pragma once

#include "response_visitor.h"
#include "scenario.h"

#include <alice/megamind/protos/scenarios/combinator_response.pb.h>

namespace NAlice::NMegamind {

void UpdateCombinatorSession(ISessionBuilder& sessionBuilder,
                             const TClientEntities& entities,
                             const TFrameActionsMap& frameActions,
                             const NMegamind::TStackEngineCore& stackEngineCore,
                             const NAlice::NScenarios::TLayout& layout,
                             const TString& productScenarioName,
                             IRunWalkerRequestCtx& walkerCtx,
                             const NScenarios::TCombinatorResponse& combinatorResponse,
                             IScenarioWalker::TPreClassifyState& preClassifyState,
                             const TString& combinatorName,
                             const TRequest& request);

void FinalizeCombinator(TResponseBuilder& builder, IRunWalkerRequestCtx& walkerCtx,
                        const TRequest& request,
                        const NScenarios::TCombinatorResponse& combinatorResponse,
                        IScenarioWalker::TPreClassifyState& preClassifyState,
                        const TString& combinatorName);

} // namespace NAlice::NMegamind
