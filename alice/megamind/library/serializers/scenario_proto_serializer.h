#pragma once

#include <alice/megamind/library/models/directives/callback_directive_model.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

namespace NAlice::NMegamind {

class TScenarioProtoSerializer final {
public:
    static NScenarios::TCallbackDirective SerializeDirective(const TCallbackDirectiveModel& model);
};

} // namespace NAlice::NMegamind
