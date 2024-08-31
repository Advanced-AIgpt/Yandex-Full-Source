#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/protos/features.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywoodFw::NVideo {

    inline NVideoCommon::TVideoFeatures MakeNullVideoFeatures() {
        NVideoCommon::TVideoFeatures features;
        features.SetIsSearchVideo(0);
        features.SetIsSelectVideoFromGallery(0);
        features.SetIsPaymentConfirmed(0);
        features.SetIsAuthorizeProvider(0);
        features.SetIsOpenCurrentVideo(0);
        features.SetIsGoToVideoScreen(0);
        features.SetAutoselect(0);
        features.SetItemSelectorConfidence(-1.0);
        features.SetItemSelectorConfidenceByName(-1.0);
        features.SetItemSelectorConfidenceByNumber(-1.0);
        return features;
    }

    inline TRunFeatures FillIntentData(const NHollywoodFw::TRunRequest& request, bool autoplay = false, const NVideoCommon::TVideoFeatures& preFeatures = {}) {
        // intent
        TString realIntent = request.AI().GetIntent();
        request.AI().OverrideResultSemanticFrame(ToString(realIntent));
        if (realIntent == ToString(NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT) || realIntent == ToString(NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_NUMBER)) {
            realIntent = ToString(NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY);
        }
        auto mmIntent = "mm." + ToString(realIntent);
        request.AI().OverrideIntent(mmIntent);

        TRunFeatures features;
        features.SetIntentName(mmIntent);

        // features
        NVideoCommon::TVideoFeatures vFeatures = MakeNullVideoFeatures();
        vFeatures.MergeFrom(preFeatures);

        if (realIntent == ToString(NVideoCommon::SEARCH_VIDEO)) {
            vFeatures.SetIsSearchVideo(true);
        } else if (realIntent == ToString(NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO)) {
            vFeatures.SetIsOpenCurrentVideo(true);
        } else if (realIntent = ToString(NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY)) {
            vFeatures.SetIsSelectVideoFromGallery(true);
            vFeatures.SetAutoselect(1);
        }
        vFeatures.SetAutoplay(autoplay);
        features.Set(vFeatures);
        return features;
    }

} // namespace NAlice::NHollywoodFw::NVideo
