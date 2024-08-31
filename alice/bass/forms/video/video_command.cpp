#include "video_command.h"
#include "utils.h"

#include <alice/bass/libs/video_common/parsers/video_item.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS::NVideo {

void AddAnalyticsInfoFromVideoCommand(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().AddObject(NAlice::NMegamind::GetAnalyticsObjectCurrentlyPlayingVideo(
        GetCurrentlyPlayingVideoForAnalyticsInfo(ctx)));
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
}

} // namespace NBASS::NVideo
