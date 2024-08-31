#pragma once

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice {

ui32 UpdateEventCount(
    TMaybe<TString> previousScenarioName,
    ui32 count,
    const TString& winnerScenarioName,
    bool eventOccured
);

} // namespace NAlice
