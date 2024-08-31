#pragma once

#include <alice/library/response_similarity/proto/similarity.pb.h>
#include <alice/library/video_common/protos/features.pb.h>
#include <alice/protos/data/tv/carousel.pb.h>
#include <library/cpp/langs/langs.h>

namespace NAlice::NHollywoodFw::NVideo {

TMaybe<NVideoCommon::TVideoFeatures> FillCarouselSimilarity(
    const NTv::TCarousel& carousel,
    const TString& requestText,
    const ELanguage& lang);

TMaybe<NVideoCommon::TVideoFeatures> FillCarouselItemSimilarity(
    const NTv::TCarouselItemWrapper& item,
    const TString& requestText,
    const ELanguage& lang);

} // NAlice::NHollywoodFw::NVideo
