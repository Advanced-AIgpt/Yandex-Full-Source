#include "analytics_info.h"

namespace NAlice::NHollywoodFw::NMusic {

NScenarios::TAnalyticsInfo::TAction CreateAction(TStringBuf id, TStringBuf name, TStringBuf humanReadable) {
    NScenarios::TAnalyticsInfo::TAction action;
    action.SetId(id.data(), id.size());
    action.SetName(name.data(), name.size());
    action.SetHumanReadable(humanReadable.data(), humanReadable.size());
    return action;
}

} // namespace NAlice::NHollywoodFw::NMusic
