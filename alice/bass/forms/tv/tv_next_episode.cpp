#include "tv_next_episode.h"

#include "tv_helper.h"

#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

constexpr TStringBuf QUASAR_NEXT_TV_EPISODE = "quasar.next_episode";
constexpr TStringBuf QUASAR_NEXT_TV_EPISODE_STUB_INTENT = "quasar.autoplay.next_episode";

TResultValue TNextTvEpisodeActionHandler::Do(NBASS::TRequestHandler& r) {
    // action means 'current tv-episode is over, need new stream'
    // will re-request actual episode stream for current channel
    auto& analyticsInfoBuilder = r.Ctx().GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::VIDEO);
    analyticsInfoBuilder.SetIntentName(TString{QUASAR_NEXT_TV_EPISODE_STUB_INTENT});
    TSchemeHolder<NVideo::TVideoItemScheme> holder = NVideo::GetCurrentVideoItem(r.Ctx());
    NVideo::TVideoItemScheme item = holder.Scheme();
    TTvChannelsHelper tvHelper(r.Ctx());
    return tvHelper.PlayCurrentTvEpisode(item);

}

void TNextTvEpisodeActionHandler::Register(NBASS::THandlersMap* handlers) {
    handlers->RegisterActionHandler(QUASAR_NEXT_TV_EPISODE, []() { return MakeHolder<TNextTvEpisodeActionHandler>(); });
}

} //namespace NBASS
