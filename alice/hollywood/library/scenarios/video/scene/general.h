#pragma once

#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NVideo {

    inline static constexpr TStringBuf NOT_SUPPORTED = "not_supported";
    inline static constexpr TStringBuf NO_TV_IS_PLUGGED_IN = "no_tv_is_plugged_in";

    class TVideoNotSupportedScene: public TScene<TNotSupportedSceneArgs> {
    public:
        TVideoNotSupportedScene(const TScenario* owner)
            : TScene(owner, NOT_SUPPORTED)
        {
            RegisterRenderer(&TVideoNotSupportedScene::Render);
        }

        inline TRetMain Main(const TNotSupportedSceneArgs& args, const TRunRequest&, TStorage&, const TSource&) const {
            return TReturnValueRender(&TVideoNotSupportedScene::Render, args);
        }

        inline TRetResponse Render(const TNotSupportedSceneArgs&, TRender& render) const {
            render.CreateFromNlg("video", "video_not_supported", NJson::TJsonValue{});
            return TReturnValueSuccess();
        }
    };

    class TNoTvPluggedScene: public TScene<TNoTvPluggedSceneArgs> {
    public:
        TNoTvPluggedScene(const TScenario* owner)
            : TScene(owner, NO_TV_IS_PLUGGED_IN)
        {
            RegisterRenderer(&TNoTvPluggedScene::Render);
        }

        inline TRetMain Main(const TNoTvPluggedSceneArgs& args, const TRunRequest&, TStorage&, const TSource&) const {
            return TReturnValueRender(&TNoTvPluggedScene::Render, args);
        }

        inline TRetResponse Render(const TNoTvPluggedSceneArgs&, TRender& render) const {
            render.CreateFromNlg("video", "please_connect_tv", NJson::TJsonValue{});
            return TReturnValueSuccess();
        }
    };

} // namespace NAlice::NHollywoodFw::NVideo
