#pragma once

#include "video_utils.h"
#include <alice/library/search_result_parser/video/parsers.h>

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>


namespace NAlice::NHollywoodFw::NVideo {

class TVideoScenarioDispatch : public TScenario {
public:
    class TVideoScenarioScene : public TScene<TVideoItem> {
    public:
        TVideoScenarioScene(const TScenario* owner)
            : TScene(owner, "scene1")
        {
        }
        TRetMain Main(const TVideoItem&, const TRunRequest&, TStorage&, const TSource&) const override {
            HW_ERROR("This scene never called");
        }
    };

    TVideoScenarioDispatch();
    static TRetResponse RenderIrrelevant(const TRenderIrrelevant&, TRender&);

private:
    TRetSetup DispatchSetup(const TRunRequest&, const TStorage&) const;
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const;

    TRetScene DispatchSceneForBaseInfo(const NHollywood::NVideo::TFrameSearchVideo& frameSearch, const NTv::TCarouselItemWrapper& baseInfo, TRTLogger& logger) const;
};
} // namespace NAlice::NHollywoodFw::NVideo
