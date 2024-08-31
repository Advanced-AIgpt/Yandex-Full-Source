#pragma once

#include <alice/bass/forms/context/context.h>

// FIXME (a-sidorin@) Move to alice/library.
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NBASS {

NAlice::TErrorOr<NAlice::NScenarios::TScenarioResponseBody>
BassContextToProtocolResponseBody(TContext& ctx, const TMaybe<TString>& nlgTemplateId);

} // namespace NBASS
