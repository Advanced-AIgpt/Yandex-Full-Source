#include "event_count.h"

namespace NAlice {

ui32 UpdateEventCount(
    TMaybe<TString> previousScenarioName,
    ui32 count,
    const TString& winnerScenarioName,
    bool eventOccured
) {
    if(!previousScenarioName.Defined()) {
        previousScenarioName = winnerScenarioName;
    }
    if (*previousScenarioName != winnerScenarioName) {
        count = 0;
    }
    if (eventOccured) {
        ++count;
    } else {
        count = 0;
    }
    return count;
}

} // namespace NAlice
