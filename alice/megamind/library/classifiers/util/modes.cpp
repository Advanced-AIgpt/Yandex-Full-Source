#include "modes.h"

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/util/ttl.h>

namespace NAlice {

bool IsActiveScenario(const ISession* session, const TStringBuf name, const ui64 activeScenarioTimeoutMs,
                      const ui64 serverTimeMs) {
    return session && session->GetPreviousScenarioName() == name &&
           session->GetPreviousScenarioSession().GetActivityTurn() > 0 &&
           !NMegamind::IsTimeoutExceeded(session->GetPreviousScenarioSession().GetTimestamp() / 1000,
                                         activeScenarioTimeoutMs, serverTimeMs);
}

bool IsModalModeAllowed(const ISession* session, i32 maxActivityTurns, const TString& name) {
    const bool infiniteActivityTurnsAllowed = maxActivityTurns == -1;
    const auto currentActivityTurn = (session && session->GetPreviousScenarioName() == name)
            ? session->GetPreviousScenarioSession().GetActivityTurn()
            : 0;
    return infiniteActivityTurnsAllowed || currentActivityTurn <= maxActivityTurns;
}

bool IsPlayerOwnerScenario(const TDeviceState& deviceState, const TStringBuf name) {
    const auto& scenarioMeta = deviceState.GetAudioPlayer().GetScenarioMeta();
    const auto* scenarioName = MapFindPtr(scenarioMeta, TString{NMegamind::SCENARIO_NAME_JSON_KEY});
    return scenarioName && *scenarioName == name;
}

} // namespace NAlice
