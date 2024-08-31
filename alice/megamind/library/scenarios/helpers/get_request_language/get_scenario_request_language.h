#pragma once

#include <alice/megamind/library/config/scenario_protos/config.pb.h>
#include <alice/megamind/library/context/context.h>

namespace NAlice::NMegamind {

ELanguage GetScenarioRequestLanguage(const TScenarioConfig& config, const IContext& ctx);

}
