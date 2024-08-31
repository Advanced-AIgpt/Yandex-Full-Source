#include "context.h"
#include "devices_settings.h"
#include "music_announce.h"

#include <alice/hollywood/library/scenarios/settings/common/names.h>
#include <alice/hollywood/library/scenarios/settings/nlg/register.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NSettings {

namespace {

using TFrameHandler = std::function<bool(const TTypedSemanticFrame&, const TSettingsRunContext&)>;

// THandler is the handler class with HandleFrame methods
#define FRAME_HANDLER(THandler, FrameName)                                                                            \
    {                                                                                                                 \
        TTypedSemanticFrame::k##FrameName##SemanticFrame,                                                             \
            TFrameHandler([](const TTypedSemanticFrame& frame, const TSettingsRunContext& ctx) -> bool {              \
                return THandler{ctx}.HandleFrame(frame.Get##FrameName##SemanticFrame());                              \
            })                                                                                                        \
    }

const THashMap<TTypedSemanticFrame::TypeCase, TFrameHandler> FRAME_HANDLERS = {
    FRAME_HANDLER(TMusicAnnounce, MusicAnnounceEnable),
    FRAME_HANDLER(TMusicAnnounce, MusicAnnounceDisable),
    FRAME_HANDLER(TDevicesSettings, OpenTandemSetting),
    FRAME_HANDLER(TDevicesSettings, OpenSmartSpeakerSetting),
};

class TSettingsHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& handleCtx) const override {
        TSettingsRunContext ctx{handleCtx};

        auto& analyticsInfo = ctx.BodyBuilder.CreateAnalyticsInfoBuilder();
        analyticsInfo.SetProductScenarioName(TString{SCENARIO_NAME});

        bool handled = false;
        for (const auto& frame : ctx.Request.Input().Proto().GetSemanticFrames()) {
            const auto& tsf = frame.GetTypedSemanticFrame();
            const auto handler = FRAME_HANDLERS.FindPtr(tsf.GetTypeCase());
            if (handler && (*handler)(tsf, ctx)) {
                analyticsInfo.SetIntentName(frame.GetName());
                handled = true;
                break;
            }
        }
        if (!handled) {
            ctx.Builder.SetIrrelevant();
            ctx.BodyBuilder.AddRenderedTextAndVoice(SCENARIO_NAME, RENDER_IRRELEVANT, ctx.NlgData);
        }

        handleCtx.ServiceCtx.AddProtobufItem(*std::move(ctx.Builder).BuildResponse(), RESPONSE_ITEM);
    }
};

} // namespace

namespace NNlg = NAlice::NHollywood::NLibrary::NScenarios::NSettings::NNlg;
REGISTER_SCENARIO("settings", AddHandle<TSettingsHandle>().SetNlgRegistration(NNlg::RegisterAll));

} // namespace NAlice::NHollywood
