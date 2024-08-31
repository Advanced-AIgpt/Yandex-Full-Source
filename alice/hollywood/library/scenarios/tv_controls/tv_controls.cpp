#include "tv_controls.h"

#include "longtap_tutorial.h"
#include "open_screensaver.h"

#include <alice/hollywood/library/scenarios/tv_controls/nlg/register.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::NTvControls {

    HW_REGISTER(TTvControlsScenario);

    static constexpr TStringBuf RENDER_INABILITY = "render_inability";
    static constexpr TStringBuf ENABLE_SCREENSAVER = "enable_screensaver";

    TTvControlsScenario::TTvControlsScenario()
        : TScenario(NProductScenarios::TV_CONTROLS)
    {
        Register(&TTvControlsScenario::Dispatch);
        RegisterScene<TTvOpenScreensaverScene>([this]() {
            RegisterSceneFn(&TTvOpenScreensaverScene::Main);
            RegisterRenderer(&TTvOpenScreensaverScene::Render);
        });
        RegisterScene<TRelevantInability>([this]() {
            RegisterSceneFn(&TRelevantInability::Main);
            RegisterRenderer(&TRelevantInability::RenderInability);
        });
        RegisterScene<TTvLongTapTutorialScene>([this]() {
            RegisterSceneFn(&TTvLongTapTutorialScene::Main);
        });
        RegisterRenderer(&TTvControlsScenario::RenderIrrelevant);
        SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTvControls::NNlg::RegisterAll);
    }

    TRetScene TTvControlsScenario::Dispatch(const TRunRequest& runRequest, const TStorage&, const TSource&) const {
        auto deviceState = runRequest.Client().TryGetMessage<TDeviceState>();
        Y_ENSURE(deviceState, "Unable to get TDeviceState");

        if (const TTvOpenScreensaverFrame screensaverFrame(runRequest.Input()); screensaverFrame.Defined() && runRequest.Flags().IsExperimentEnabled(ENABLE_SCREENSAVER)) {
            if (IsTvOrModuleOrTandemRequest(runRequest, *deviceState) &&
                (SupportsScreensaver(runRequest, *deviceState) || runRequest.Flags().IsExperimentEnabled(IGNORE_SCREENSAVER_CAPABILITY))) {
                LOG_INFO(runRequest.Debug().Logger()) << "Found " << screensaverFrame.GetName() << " frame";
                return TReturnValueScene<TTvOpenScreensaverScene>(TTvOpenScreensaverArgs{}, screensaverFrame.GetName());
            } else {
                return TReturnValueScene<TRelevantInability>(TTvControlsRenderNotSupported{}, screensaverFrame.GetName());
            }
        }

        if (TTvLongTapTutorialSemanticFrame longTapTsf; runRequest.Input().FindTSF(longTapTsf)) {
            return TReturnValueScene<TTvLongTapTutorialScene>(TTvLongTapTutorialArgs{}, ToString(LONGTAP_TUTORIAL_FRAME));
        }

        return TReturnValueRenderIrrelevant(&TTvControlsScenario::RenderIrrelevant, {});
    }

    TRetResponse TTvControlsScenario::RenderIrrelevant(const TTvControlsRenderIrrelevant&, TRender& render) {
        render.CreateFromNlg("tv_controls", "notsupported", NJson::TJsonValue{});
        return TReturnValueSuccess();
    }

    TRelevantInability::TRelevantInability(const TScenario* owner)
        : TScene(owner, RENDER_INABILITY)
    {
    }

    TRetMain TRelevantInability::Main(const TTvControlsRenderNotSupported& args, const TRunRequest&, TStorage&, const TSource&) const {
        return TReturnValueRender(&TRelevantInability::RenderInability, args);
    }

    TRetResponse TRelevantInability::RenderInability(const TTvControlsRenderNotSupported&, TRender& render) {
        render.CreateFromNlg("tv_controls", "notsupported", NJson::TJsonValue{});
        return TReturnValueSuccess();
    }
}
