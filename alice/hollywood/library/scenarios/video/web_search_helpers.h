#pragma once

#include <alice/library/logger/logger.h>
#include <alice/hollywood/library/framework/framework.h>

#include <alice/protos/data/search_result/tv_search_result.pb.h>
#include <alice/protos/data/tv/carousel.pb.h>

namespace NAlice::NHollywoodFw::NVideo::WebSearchHelpers {
    bool HasUsefulSnippet(const TRunRequest& request, TRTLogger& logger);

    TTvSearchResultData ParseSearchSnippetFromWeb(const TRunRequest& request, TRTLogger& logger);

    TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfoFromWeb(const TRunRequest& request, TRTLogger& logger);
}