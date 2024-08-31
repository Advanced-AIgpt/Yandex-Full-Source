#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>


namespace NAlice::NHollywoodFw::NVideo {

inline constexpr TStringBuf BASS_PROXY = "bass_proxy";

class TVideoBassProxy : public TScene<TBassProxySceneArgs> {
public:
    TVideoBassProxy(const TScenario *owner)
        : TScene(owner, BASS_PROXY)
    {
        RegisterRenderer(&TVideoBassProxy::RenderRun);
    }

    TRetSetup RunSetup(
        const TBassProxySceneArgs&,
        const TRunRequest&,
        const TStorage&) const;

    TRetMain Main(
        const TBassProxySceneArgs&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const;

    TRetResponse RenderRun(
        const NScenarios::TScenarioRunResponse&,
        TRender&) const;
};

} // namespace NAlice::NHollywoodFw::NVideo
