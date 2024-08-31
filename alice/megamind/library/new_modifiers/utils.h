#pragma once

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/library/config/scenario_protos/config.pb.h>
#include <alice/megamind/protos/modifiers/modifier_request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/network/headers.h>

namespace NAlice::NMegamind::NModifiers {

TModifierRequest ConstructModifierRequest(const NScenarios::TScenarioResponseBody& responseBody, const IContext& ctx,
                                          const TRequest& request, const TScenarioConfig& scenarioConfig);

} // namespace NAlice::NMegamind::NModifiers
