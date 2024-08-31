#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>


namespace NAlice::NHollywoodFw::NVideo {

inline constexpr TStringBuf VIDEO_PLAY_SCENE = "video_play";

class TVideoPlayScene : public TScene<TVideoVhArgs> {
public:
    TVideoPlayScene(const TScenario *owner) : TScene(owner, VIDEO_PLAY_SCENE) {}

    TRetSetup MainSetup(
        const TVideoVhArgs&,
        const TRunRequest&,
        const TStorage&) const;

    TRetMain Main(
        const TVideoVhArgs&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const;

    static TRetResponse PlayRender(
            const TVideoPlaySceneArgs &args,
            TRender &render);
    static TRetResponse DetailsScreenOpen(
            const TVideoDetailsScreenArgs &args,
            TRender &render);
    static TRetResponse PirateVideoOpen(
            const TVideoItem &item,
            TRender &render);
};

} // namespace NAlice::NHollywoodFw::NVideo
