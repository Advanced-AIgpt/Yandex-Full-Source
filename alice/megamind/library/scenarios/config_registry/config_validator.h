#pragma once

#include <alice/megamind/library/config/scenario_protos/config.pb.h>
#include <alice/megamind/library/config/scenario_protos/combinator_config.pb.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

void ValidateScenarioConfig(const TScenarioConfig& config);
void ValidateCombinatorConfig(const TCombinatorConfigProto& config);

} // namespace NAlice::NMegamind
