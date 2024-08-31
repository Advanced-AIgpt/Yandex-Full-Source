#pragma once

#include <alice/megamind/library/session/protos/state.pb.h>

namespace NAlice::NMegamind {

struct TScenarioSessionView {
    const TState& ScenarioState;
    bool IsNewSession = true;
    bool IsSessionReset = false;
};

} // namespace NAlice::NMegamind
