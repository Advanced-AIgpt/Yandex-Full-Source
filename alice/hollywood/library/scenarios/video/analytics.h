#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/analytics/scenarios/video/video.pb.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>
#include <alice/protos/data/tv/carousel.pb.h>
#include <alice/protos/data/video/video.pb.h>


namespace NAlice::NHollywood::NVideo {


inline NScenarios::IAnalyticsInfoBuilder& MakeVideoScenarioAnalytics(TResponseBodyBuilder& bodyBuilder, TStringBuf intent) {
        auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
        analyticsInfoBuilder.SetIntentName(TString{intent});
        analyticsInfoBuilder.SetProductScenarioName("video");
        return analyticsInfoBuilder;
}

NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForDescription(const TVideoItem& item, bool withPayScreen);

NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForGallery(const TVector<TVideoItem>& items);

NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForTvSearch(const TTvSearchResultData& items);

NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectForSeasonGallery(const TVideoItem& tvShowItem, const TVideoItem& items, ui32 seasonNumber);

NScenarios::TAnalyticsInfo::TObject GetAnalyticsObjectCurrentlyPlayingVideo(const TVideoItem& item);

} // namespace NAlice::NHollywood::NVideo
