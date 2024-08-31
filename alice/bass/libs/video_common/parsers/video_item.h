#pragma once

#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

namespace NSc {
class TValue;
} // namespace NSc

namespace NAlice::NMegamind {

NAlice::NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForDescription(const NSc::TValue& item, bool withPayScreen = false);

NAlice::NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForGallery(const NSc::TValue& items);

NAlice::NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForSeasonGallery(const NSc::TValue& tvShowItem,
                                                                               const NSc::TValue& items,
                                                                               ui32 seasonNumber);

NAlice::NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectCurrentlyPlayingVideo(const NSc::TValue& item);

} // namespace NAlice::NMegamind
