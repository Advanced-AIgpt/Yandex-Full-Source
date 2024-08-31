#pragma once

#include <alice/hollywood/library/scenarios/tv_controls/proto/tv_controls.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NTvControls {

    inline static constexpr TStringBuf LONGTAP_TUTORIAL = "longtap_tutorial";
    inline static constexpr TStringBuf LONGTAP_TUTORIAL_FRAME = "alice.tv.long_tap_tutorial";

    class TTvLongTapTutorialScene: public TScene<TTvLongTapTutorialArgs> {
    public:
        TTvLongTapTutorialScene(const TScenario* owner)
            : TScene(owner, LONGTAP_TUTORIAL)
        {
            RegisterRenderer(&TTvLongTapTutorialScene::Render);
        }

        TRetMain Main(const TTvLongTapTutorialArgs&, const TRunRequest&, TStorage&, const TSource&) const override {
            return TReturnValueRender(&TTvLongTapTutorialScene::Render, {});
        }

        TRetResponse Render(const TTvLongTapTutorialArgs& args, TRender& render) const {
            render.CreateFromNlg("tv_controls", "longtap_tutorial", args);
            return TReturnValueSuccess();
        }
    };

}
