#pragma once

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/action/tv_action.pb.h>
#include <alice/protos/data/video/video.pb.h>
#include <alice/protos/data/tv/carousel.pb.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>

namespace NAlice::NHollywoodFw::NVideo {
class TVideoVoiceButtons {
    public:
    static void SetupTvActions(NAlice::TTvSearchCarouselWrapper& carousel);
    static void SetupTvActions(NAlice::TTvSearchResultData& searchResultData);
};
}