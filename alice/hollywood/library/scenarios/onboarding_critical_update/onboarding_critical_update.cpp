#include "onboarding_critical_update.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>

#include <alice/library/experiments/experiments.h>

#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf FRAME_CONFIGURE_SUCCESS = "alice.onboarding.starting_configure_success";
constexpr TStringBuf FRAME_CRITICAL_UPDATE = "alice.onboarding.starting_critical_update";
constexpr TStringBuf SCENARIO = "onboarding_critical_update";

constexpr TStringBuf EXP_DEBUG_BOOL_OVERRIDE_PREFIX = "onboarding_debug_override_";
constexpr TStringBuf EXP_DEBUG_IGNORE_CURRENT_DEVICE = "onboarding_debug_ignore_current_device";
constexpr TStringBuf EXP_DEBUG_IGNORE_NO_IOT = "onboarding_debug_ignore_no_iot";
constexpr TStringBuf EXP_REDIRECT_TO_MUSIC_ONBOARDING = "onboarding_redirect_to_music_onboarding";

const TString ACTION_NAME_CONFIRM = "onboarding_confirm";
const TString ACTION_NAME_DECLINE = "onboarding_decline";

const TString HINT_FRAME_CONFIRM = "alice.proactivity.confirm";
const TString HINT_FRAME_DECLINE = "alice.proactivity.decline";

class TOnboardingRunner {
public:
    TOnboardingRunner(TScenarioHandleContext& ctx)
        : Ctx{ctx}
        , Logger{ctx.Ctx.Logger()}
        , RequestProto{GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM)}
        , Request{RequestProto, ctx.ServiceCtx}
        , Client{Request.ClientInfo()}
        , FrameConfigureSuccess{Request.Input().FindSemanticFrame(FRAME_CONFIGURE_SUCCESS)}
        , FrameCriticalUpdate{Request.Input().FindSemanticFrame(FRAME_CRITICAL_UPDATE)}
        , Intent(FrameConfigureSuccess ? FRAME_CONFIGURE_SUCCESS : FRAME_CRITICAL_UPDATE)
        , NlgData{Logger, Request}
        , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
        , Builder{&NlgWrapper}
        , BodyBuilder{Builder.CreateResponseBodyBuilder()}
    {
        Y_ENSURE(FrameConfigureSuccess || FrameCriticalUpdate, "Have not found any expected OnboardingStarting frame");
        Y_ENSURE(!FrameConfigureSuccess || !FrameCriticalUpdate, "Expected one OnboardingStarting frame but found two");
        BodyBuilder.CreateAnalyticsInfoBuilder()
            .SetIntentName(Intent)
            .SetProductScenarioName(TString{SCENARIO});
    }

    void Run() {
        LOG_INFO(Logger) << "Intent: " << Intent;

        Y_ASSERT(Client.IsSmartSpeaker());

        if (FrameCriticalUpdate) {
            RenderCriticalUpdate();
        } else if (FrameConfigureSuccess) {
            RenderConfigureSuccess();
        } else {
            Y_UNREACHABLE();
        }
    }

private:
    TScenarioHandleContext& Ctx;
    TRTLogger& Logger;
    const TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    const TClientInfo& Client;
    const TPtrWrapper<NAlice::TSemanticFrame> FrameConfigureSuccess;
    const TPtrWrapper<NAlice::TSemanticFrame> FrameCriticalUpdate;
    const TString Intent;

    TNlgData NlgData;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TResponseBodyBuilder& BodyBuilder;

    // For configure success phrase tree
    bool HasPlus;
    bool TvPlugged;
    bool SupportsVideo;
    bool FirstActivation;
    bool CanConnectRemote;
    bool ManySpeakers;

    bool AddedConfirmAction = false;

private:
    void RenderPhrase(const TString& phrase) {
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(SCENARIO, phrase, /* buttons = */ {}, NlgData);
        LOG_INFO(Logger) << "Added phrase " << phrase;
    }

    void RenderConfigureSuccessPhrase(const TString& phrase) {
        RenderPhrase(TString::Join("render_configure_success__", phrase));
    }

    using TTypedFrameSetter = std::function<void(TTypedSemanticFrame&)>;
    void AddFrameAction(const TString& actionName, const TString& hintFrame, const TString& logResultFrame, const TTypedFrameSetter& frameSetter) {
        NScenarios::TFrameAction action;
        action.MutableNluHint()->SetFrameName(hintFrame);

        auto& parsed = *action.MutableParsedUtterance();
        parsed.MutableAnalytics()->SetPurpose(actionName);
        frameSetter(*parsed.MutableTypedSemanticFrame());

        BodyBuilder.AddAction(actionName, std::move(action));
        LOG_INFO(Logger) << "Added frame action " << actionName << ": " << hintFrame << " -> " << logResultFrame;
    }

    void AddConfirmAction(const TString& logResultFrame, const TTypedFrameSetter& frameSetter) {
        Y_ASSERT(!AddedConfirmAction);
        AddedConfirmAction = true;
        AddFrameAction(ACTION_NAME_CONFIRM, HINT_FRAME_CONFIRM, logResultFrame, frameSetter);
    }

    void WrapResponse() {
        auto response = std::move(Builder).BuildResponse();
        Ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        LOG_DEBUG(Logger) << "Wrapped response";
    }

    void WrapSuccess() {
        BodyBuilder.AddClientActionDirective("success_starting_onboarding", {});
        BodyBuilder.AddClientActionDirective("tts_play_placeholder", {});
        LOG_DEBUG(Logger) << "Added success directives";
        WrapResponse();
    }

    void WrapDefaultOnError(const TString& error) {
        LOG_ERR(Logger) << error;
        LOG_INFO(Logger) << "Fallback to default response";
        RenderPhrase("render_default");
        WrapSuccess();
    }

    void InitBool(bool& toInit, const TString& name, const bool value) {
        const auto expValue = GetExperimentValueWithPrefix(
            Request.ExpFlags(),
            TString::Join(EXP_DEBUG_BOOL_OVERRIDE_PREFIX, name, "=")
        );
        toInit = expValue.Defined() ? FromString<bool>(expValue.GetRef()) : value;
        NlgData.Context[name] = toInit;
        LOG_INFO(Logger) << name << ": " << (toInit ? "YES" : "NO");
    }

    bool TryFillConfigureSuccessData() {
        const auto* iotUserInfo = Request.GetDataSource(EDataSourceType::IOT_USER_INFO);
        if (!iotUserInfo && !Request.HasExpFlag(EXP_DEBUG_IGNORE_NO_IOT)) {
            WrapDefaultOnError("Did not get required data source IOT_USER_INFO");
            return false;
        }

        TIoTUserInfo debugIotUserInfo;
        const auto& devices = iotUserInfo ? iotUserInfo->GetIoTUserInfo().GetDevices() : debugIotUserInfo.GetDevices();
        const auto* currentDevice = FindIfPtr(devices, [&] (const auto& device) {
            return device.GetQuasarInfo().GetDeviceId() == Client.DeviceId;
        });
        if (!currentDevice && !Request.HasExpFlag(EXP_DEBUG_IGNORE_CURRENT_DEVICE)) {
            WrapDefaultOnError("Could not find current speaker in IOT_USER_INFO");
            return false;
        }

        const auto* userInfo = GetUserInfoProto(Request);
        InitBool(HasPlus, "HasPlus", userInfo ? userInfo->GetHasYandexPlus() : false);

        InitBool(SupportsVideo, "SupportsVideo", Request.Interfaces().GetSupportsHDMIOutput());
        InitBool(TvPlugged, "TvPlugged", Request.Interfaces().GetIsTvPlugged());

        // Added to account less than 10 minutes ago
        // !currentDevice should only happen when EXP_DEBUG_IGNORE_CURRENT_DEVICE is present
        InitBool(FirstActivation, "FirstActivation", currentDevice ? (Client.Epoch - currentDevice->GetCreated() < 10 * 60) : false);

        const auto& deviceState = Request.BaseRequestProto().GetDeviceState();
        InitBool(CanConnectRemote, "CanConnectRemote", Request.Interfaces().GetSupportsBluetoothRCU() &&
                                                       !deviceState.GetRcuState().GetIsRcuConnected());

        const auto speakerCount = CountIf(devices, [&](const auto& device) { return device.HasQuasarInfo(); });
        LOG_INFO(Logger) << "SpeakerCount: " << speakerCount;
        InitBool(ManySpeakers, "ManySpeakers", speakerCount > 1);

        return true;
    }

    void RenderCriticalUpdate() {
        RenderPhrase("render_critical_update");
        WrapSuccess();
    }

    void RenderConfigureSuccess() {
        if (!TryFillConfigureSuccessData()) {
            // Already wrapped failure inside
            return;
        }

        if (!HasPlus) {
            RenderConfigureSuccessPhrase("no_plus");
            // Confirm action "как подключить плюс"
            AddConfirmAction("how_to_subscribe", [](auto& frame) {
                frame.MutableHowToSubscribeSemanticFrame();
            });
        } else if (SupportsVideo && !TvPlugged) {
            RenderConfigureSuccessPhrase("connect_hdmi");
        } else if (SupportsVideo && CanConnectRemote) {
            RenderConfigureSuccessPhrase("connect_remote");
        } else if (!FirstActivation) {
            RenderConfigureSuccessPhrase("reconfigure");
        } else if (SupportsVideo) {
            RenderConfigureSuccessPhrase("show_series");
            // Confirm action "покажи новые сериалы"
            AddConfirmAction("video_play new series", [](auto& frame) {
                auto& videoFrame = *frame.MutableVideoPlaySemanticFrame();
                videoFrame.MutableAction()->SetStringValue("play");
                videoFrame.MutableContentType()->SetStringValue("tv_show");
                videoFrame.MutableNew()->SetNewValue("new_video");
            });
        } else if (ManySpeakers) {
            RenderConfigureSuccessPhrase("play_music_everywhere");
            // Confirm action "включи музыку везде"
            AddConfirmAction("music_play on all devices", [](auto& frame) {
                auto& musicFrame = *frame.MutableMusicPlaySemanticFrame();
                musicFrame.MutableLocation()->SetUserIotMultiroomAllDevicesValue("everywhere");
            });
        } else {
            if (Request.HasExpFlag(EXP_REDIRECT_TO_MUSIC_ONBOARDING)) {
                RenderConfigureSuccessPhrase("music_onboarding");
                // Confirm action "настрой музыкальные рекомендации"
                AddConfirmAction("music_onboarding", [](auto& frame) {
                    frame.MutableMusicOnboardingSemanticFrame();
                });
            } else {
                RenderConfigureSuccessPhrase("play_music");
                // Confirm action "включи музыку"
                AddConfirmAction("music_play", [](auto& frame) {
                    frame.MutableMusicPlaySemanticFrame();
                });
            }
        }

        if (AddedConfirmAction) {
            // Decline action
            AddFrameAction(ACTION_NAME_DECLINE, HINT_FRAME_DECLINE, "do_nothing", [](auto& frame) {
                frame.MutableDoNothingSemanticFrame();
            });
            BodyBuilder.SetShouldListen(true);
        }

        WrapSuccess();
    }
};

} // namespace

void TOnboardingCriticalUpdateRunHandle::Do(TScenarioHandleContext& ctx) const {
    TOnboardingRunner{ctx}.Run();
}

REGISTER_SCENARIO("onboarding_critical_update",
                  AddHandle<TOnboardingCriticalUpdateRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NOnboardingCriticalUpdate::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
