#pragma once

#include "utils.h"

#include <alice/hollywood/library/scenarios/tv_controls/proto/tv_controls.pb.h>

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywoodFw::NTvControls {

    inline static constexpr TStringBuf OPEN_SCREENSAVER_SCENE = "open_screensaver";
    inline static constexpr TStringBuf FRAME_CONTROLS_OPEN_SCREENSAVER = "alice.controls.open_screensaver";
    inline static constexpr TStringBuf IGNORE_SCREENSAVER_CAPABILITY = "ignore_screensaver_capability";

    inline NScenarios::TAnalyticsInfo_TAction MakeScreensaverAction() {
        NScenarios::TAnalyticsInfo_TAction action;
        action.SetId("open_screensaver");
        action.SetName("open_screensaver");
        action.SetHumanReadable("Запускается заставка");
        return action;
    }

    struct TTvOpenScreensaverFrame: public TFrame {
        TTvOpenScreensaverFrame(const TRequest::TInput& input)
            : TFrame(input, FRAME_CONTROLS_OPEN_SCREENSAVER)
        {
        }
    };

    class TTvOpenScreensaverScene: public TScene<TTvOpenScreensaverArgs> {
    public:
        TTvOpenScreensaverScene(const TScenario* owner)
            : TScene(owner, OPEN_SCREENSAVER_SCENE)
        {
        }

        TRetMain Main(const TTvOpenScreensaverArgs& args, const TRunRequest& request, TStorage&, const TSource&) const override {
            request.AI().AddAction(MakeScreensaverAction());
            return TReturnValueRender(&TTvOpenScreensaverScene::Render, args);
        }

        static TRetResponse Render(const TTvOpenScreensaverArgs& args, TRender& render) {
            render.CreateFromNlg("tv_controls", "open_screensaver", args);
            render.Directives().AddOpenScreensaverDirective();
            return TReturnValueSuccess();
        }
    };

}
