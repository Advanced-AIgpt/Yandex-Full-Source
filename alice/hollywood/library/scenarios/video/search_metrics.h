#pragma once

#include <alice/library/logger/logger.h>
#include <alice/hollywood/library/framework/framework.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>
#include <alice/protos/data/tv/carousel.pb.h>

namespace NAlice::NHollywoodFw::NVideo::NSearchMetrics {
    void TrackWebSearchEntitySnippet(const TRunRequest& request);

    enum ESearchResultSource {
        WebSearchBaseInfo,
        WebSearchAll,
        VideoSearchBaseInfo,
        VideoSearchAll,
        Other
    };
    void TrackWhichSearchResultWasUsed(NMetrics::ISensors& sensors, enum ESearchResultSource source);

    void TrackVideoSearchResponded(NMetrics::ISensors& sensors, bool success);
    void TrackVideoSearchResult(NMetrics::ISensors& sensors, const TMaybe<TTvSearchResultData>& result);
    void TrackVideoSearchBaseInfo(NMetrics::ISensors& sensors, const TMaybe<NTv::TCarouselItemWrapper> baseInfo);
}
