#include "pause.h"

#include "common.h"
#include "frame_redirect.h"

#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/s3_animations/s3_animations.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>

#include <util/generic/vector.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NPause {

namespace {

constexpr TStringBuf PAUSE_CANCEL_SEMANTIC_FRAME = "personal_assistant.scenarios.player.pause_cancel";
// TODO(zhigan): remove copypaste
constexpr TStringBuf DRAW_LED_SCREEN_DIRECTIVE_NAME = "draw_led_screen";
constexpr TStringBuf PLAYER_PAUSE_S3_ANIMATION_PATH = "animations/player_pause";
const TString PLAYER_PAUSE_GIF_URI = "https://static-alice.s3.yandex.net/led-production/player/pause.gif";

NScenarios::TDirective BuildDrawLedScreenDirectiveWithPause() {
    NScenarios::TDirective directive{};
    auto& drawLedScreenDirective = *directive.MutableDrawLedScreenDirective();

    drawLedScreenDirective.SetName(TString(DRAW_LED_SCREEN_DIRECTIVE_NAME));

    auto& drawItem = *drawLedScreenDirective.AddDrawItem();
    drawItem.SetFrontalLedImage(PLAYER_PAUSE_GIF_URI);
    return directive;
}

void ProcessPlayerPauseCommand(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                               const TVector<std::pair<TStringBuf, TStringBuf>>& directiveNameAndCmdSubName,
                               NJson::TJsonValue directiveValue, TResponseBodyBuilder& bodyBuilder) {
    if (!request.ClientInfo().IsSmartSpeaker() && !request.ClientInfo().IsTvDevice()) {
        TNlgData nlgData{logger, request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(PLAYER_PAUSE_NLG,
                                                       RENDER_PLAYER_PAUSE,
                                                       /* buttons = */ {}, nlgData);
    }

    const TCapabilityWrapper<TScenarioRunRequestWrapper> capabilityWrapper(
        request,
        GetEnvironmentStateProto(request)
    );

    if (capabilityWrapper.HasLedDisplay()) {
        bodyBuilder.AddDirective(BuildDrawLedScreenDirectiveWithPause());
    } else if (request.Interfaces().GetHasScledDisplay()) {
        NScledAnimation::AddStandardScled(bodyBuilder, NScledAnimation::EScledAnimations::SCLED_ANIMATION_PAUSE);
        bodyBuilder.AddTtsPlayPlaceholderDirective();
    } else if (capabilityWrapper.SupportsS3Animations()) {
        bodyBuilder.AddDirective(BuildDrawAnimationDirective(PLAYER_PAUSE_S3_ANIMATION_PATH));
    }

    for (const auto& [directiveName, commandSubName] : directiveNameAndCmdSubName) {
        bodyBuilder.AddClientActionDirective(TString{directiveName}, TString{commandSubName}, directiveValue);
    }
    FillAnalyticsInfo(bodyBuilder, PLAYER_PAUSE_INTENT, NProductScenarios::STOP);
}

void ProcessPlayerPauseCommand(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                               TStringBuf directiveName, TStringBuf commandSubName, NJson::TJsonValue directiveValue,
                               TResponseBodyBuilder& bodyBuilder) {
    TVector<std::pair<TStringBuf, TStringBuf>> directives{std::make_pair(directiveName, commandSubName)};
    ProcessPlayerPauseCommand(logger, request, directives, directiveValue, bodyBuilder);
}

void ProcessAlarmStopCommand(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder) {
    const bool isQuasar = request.ClientInfo().IsSmartSpeaker();
    bodyBuilder.AddClientActionDirective(TString{ALARM_STOP_DIRECTIVE},
                                         TString{isQuasar ? ALARM_STOP_ON_QUASAR_COMMAND : ALARM_STOP_COMMAND},
                                         {});
    FillAnalyticsInfo(bodyBuilder, ALARM_STOP_INTENT, NProductScenarios::ALARM);
}

void ProcessTimerStopCommand(TResponseBodyBuilder& bodyBuilder, const TVector<TString>& timerIds) {
    NJson::TJsonValue value;
    for (const auto& timerId : timerIds) {
        value["timer_id"] = timerId;
        bodyBuilder.AddClientActionDirective(TString{TIMER_STOP_DIRECTIVE}, TString{TIMER_STOP_COMMAND}, value);
    }
    FillAnalyticsInfo(bodyBuilder, TIMER_STOP_INTENT, NProductScenarios::TIMER);
}

void ProcessYaAutoStopCommand(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder) {
    FillAnalyticsInfo(bodyBuilder, PLAYER_PAUSE_INTENT, NProductScenarios::STOP);
    NJson::TJsonValue value;
    if (request.ClientInfo().IsOldYaAuto()) {
        if (request.Input().FindSemanticFrame(PAUSE_CANCEL_SEMANTIC_FRAME)) {
            value["application"] = "yandexnavi";
            value["intent"] = "external_confirmation";
            value["params"]["app"] = "yandexnavi";
            value["params"]["confirmed"] = "0";
            bodyBuilder.AddClientActionDirective(TString{YANDEX_NAVI_DIRECTIVE},
                                                 TString{YANDEX_NAVI_EXTERNAL_CONFIRMATION}, value);
        } else {
            value["application"] = "car";
            value["intent"] = "media_control";
            value["params"]["action"] = "pause";
            bodyBuilder.AddClientActionDirective(TString{CAR_DIRECTIVE}, TString{CAR_MEDIA_CONTROL}, value);
        }
    } else {
        value["uri"] = "yandexauto://media_control?action=pause";
        bodyBuilder.AddClientActionDirective(TString{OPEN_URI_DIRECTIVE}, TString{AUTO_MEDIA_CONTROL_COMMAND}, value);
    }
}

void ProcessNavigatorCancelConfirmation(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                                        TResponseBodyBuilder& bodyBuilder) {
    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(PLAYER_PAUSE_NLG,
                                                   RENDER_NAVIGATOR_CANCEL_CONFIRMATION,
                                                   /* buttons = */ {}, nlgData);
    NJson::TJsonValue value;
    value["uri"] = "yandexnavi://external_confirmation?confirmed=0";
    bodyBuilder.AddClientActionDirective(TString{OPEN_URI_DIRECTIVE}, TString{NAVI_CONFIRM_COMMAND}, value);
    FillAnalyticsInfo(bodyBuilder, PLAYER_PAUSE_INTENT, NProductScenarios::STOP);
}

void ProcessPowerOffCommand(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                            TResponseBodyBuilder& bodyBuilder) {
    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(POWER_OFF_NLG,
                                                   RENDER_POWER_OFF,
                                                   /* buttons = */ {}, nlgData);
    bodyBuilder.AddClientActionDirective(TString{POWER_OFF_DIRECTIVE},
                                         TString{POWER_OFF_DIRECTIVE_SUB_NAME}, /* value= */ {});
    FillAnalyticsInfo(bodyBuilder, POWER_OFF_INTENT, NProductScenarios::COMMANDS_OTHER);
}

void ProcessGoHomeCommand(TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TDirective directive;
    directive.MutableGoHomeDirective();
    bodyBuilder.AddDirective(std::move(directive));
    FillAnalyticsInfo(bodyBuilder, GO_HOME_INTENT, NProductScenarios::COMMANDS_OTHER);
}

void AddOnlySuggests(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                     TResponseBodyBuilder& bodyBuilder) {
    if (!request.ClientInfo().IsSmartSpeaker()) {
        TNlgData nlgData{logger, request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(PLAYER_PAUSE_NLG,
                                                       RENDER_IS_NOT_SMART_SPEAKER,
                                                       /* buttons = */ {}, nlgData);
    }
    FillAnalyticsInfo(bodyBuilder, PLAYER_PAUSE_INTENT, NProductScenarios::STOP);
    // TODO Add suggests.
}

void MakeUnsupportedResponse(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                             TResponseBodyBuilder& bodyBuilder) {
    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(PLAYER_PAUSE_NLG,
                                                   RENDER_UNSUPPORTED_OPERATION,
                                                   /* buttons = */ {}, nlgData);
    FillAnalyticsInfo(bodyBuilder, PLAYER_PAUSE_INTENT, NProductScenarios::STOP);
    // TODO Add suggests.
}

bool InStrokaOrYabro(const TClientInfo& clientInfo, bool hasStrokaYabroExp) {
    return clientInfo.IsYaStroka() ||
           (clientInfo.IsYaBrowserDesktop() && clientInfo.IsWindows() && hasStrokaYabroExp);
}

bool CanProcessGoHome(const TClientInfo& clientInfo, const TScenarioRunRequestWrapper& request) {
    return clientInfo.IsSmartSpeaker() || request.Proto().GetBaseRequest().GetInterfaces().GetSupportsGoHomeDirective();
}

bool CheckVideoPauseCapability(const TDeviceState::TVideo& videoState, const TClientInfo& clientInfo) {
    if (clientInfo.IsTvDevice()) {
        return FindPtr(videoState.GetPlayerCapabilities(), TDeviceState_TVideo_EPlayerCapability_pause) != nullptr;
    }
    // only tv has different players with its own player capabilities
    return true;
}

} // namespace

TMaybe<TFrame> GetPauseFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input,
                             const TClientInfo& clientInfo, const TScenarioRunRequestWrapper& request)
{
    if (frame.Defined() && IsIn({PAUSE_FRAME, POWER_OFF_FRAME, GO_HOME_FRAME}, frame->Name())) {
        return frame;
    }
    if (input.FindSemanticFrame(POWER_OFF_FRAME) &&
        InStrokaOrYabro(clientInfo, request.HasExpFlag(EXP_STROKA_YABRO)))
    {
        return input.CreateRequestFrame(POWER_OFF_FRAME);
    }
    if (input.FindSemanticFrame(PAUSE_FRAME)) {
        return input.CreateRequestFrame(PAUSE_FRAME);
    }
    if (input.FindSemanticFrame(GO_HOME_FRAME) && CanProcessGoHome(clientInfo, request)) {
        return input.CreateRequestFrame(GO_HOME_FRAME);
    }
    return Nothing();
}

void ProcessFastPauseCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame) {
    const auto& request = fastCommandScenarioRunContext.Request;
    auto& logger = fastCommandScenarioRunContext.Logger;

    LOG_INFO(logger) << "ProcessFastPauseCommand started...";
    auto& bodyBuilder = fastCommandScenarioRunContext.RunResponseBuilder.CreateResponseBodyBuilder(&frame);

    if (frame.Name() == POWER_OFF_FRAME) {
        ProcessPowerOffCommand(logger, request, bodyBuilder);
        return;
    }

    if (frame.Name() == GO_HOME_FRAME && request.Proto().GetBaseRequest().GetInterfaces().GetSupportsGoHomeDirective()) {
        ProcessGoHomeCommand(bodyBuilder);
        return;
    }

    const auto& deviceState = request.Proto().GetBaseRequest().GetDeviceState();

    NJson::TJsonValue directiveValue;

    if (request.Interfaces().GetMultiroom() && FrameHasSomeLocationSlot(frame)) {
#ifdef MEGAMIND_AND_APPHOST_RELEASED
        const auto multiroom = TMultiroom{request};
        const auto locationInfo = MakeLocationInfo(frame);
        const bool hasAnyDeviceInLocation = multiroom.IsDeviceInLocation(locationInfo) ||
            multiroom.FindVisiblePeerFromLocationInfo(locationInfo).Defined();

        if (!hasAnyDeviceInLocation) {
            LOG_INFO(logger) << "Can't generate player_pause directive, there are no devices in target location";
            TNlgData nlgData{logger, request};
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(PLAYER_PAUSE_NLG,
                                                           RENDER_DONT_KNOW_PLACE,
                                                           /* buttons = */ {}, nlgData);
            FillAnalyticsInfo(bodyBuilder, PLAYER_PAUSE_INTENT, NProductScenarios::STOP);
            return;
        }
#endif // MEGAMIND_AND_APPHOST_RELEASED

        AddRedirectToLocation(directiveValue, frame);

        ProcessPlayerPauseCommand(logger, request, PLAYER_PAUSE_DIRECTIVE, PLAYER_PAUSE_COMMAND,
                                  std::move(directiveValue), bodyBuilder);
        LOG_INFO(logger) << "Generated directive: player_pause with room_id";
        return;
    }

    AddMultiroomSessionIdToDirectiveValue(directiveValue, deviceState);

    if (request.ClientInfo().IsSmartSpeaker()) {
        if (deviceState.GetAlarmState().GetCurrentlyPlaying()) {
            ProcessAlarmStopCommand(request, bodyBuilder);
            return;
        }

        TVector<TString> activeTimersPlaying;
        for (const auto& activeTimer : deviceState.GetTimers().GetActiveTimers()) {
            if (activeTimer.GetCurrentlyPlaying()) {
                activeTimersPlaying.emplace_back(activeTimer.GetTimerId());
            }
        }
        if (!activeTimersPlaying.empty()) {
            ProcessTimerStopCommand(bodyBuilder, activeTimersPlaying);
            return;
        }
    }

    if (request.Interfaces().GetSupportsVideoPlayer()) {
        const auto& videoPlayer = deviceState.GetVideo().GetPlayer();
        if (videoPlayer.HasPause() && !videoPlayer.GetPause()) {
            if (!CheckVideoPauseCapability(deviceState.GetVideo(), request.ClientInfo())) {
                MakeUnsupportedResponse(logger, request, bodyBuilder);
                return;
            }

            if (!request.Interfaces().GetSupportsPlayerPauseDirective()) {
                MakeUnsupportedResponse(logger, request, bodyBuilder);
            } else {
                ProcessPlayerPauseCommand(logger, request, PLAYER_PAUSE_DIRECTIVE, PLAYER_PAUSE_COMMAND,
                                          std::move(directiveValue), bodyBuilder);
            }
            return;
        }
    }

    bool isWaitingForRouteConfirmation =
        IsIn(deviceState.GetNavigator().GetStates(), WAITING_FOR_ROUTE_NAVIGATOR_STATE);

    if (request.ClientInfo().IsNavigator()) {
        if (isWaitingForRouteConfirmation) {
            ProcessNavigatorCancelConfirmation(logger, request, bodyBuilder);
            return;
        }

        if (deviceState.HasMusic()) {
            if (!request.Interfaces().GetSupportsPlayerPauseDirective()) {
                MakeUnsupportedResponse(logger, request, bodyBuilder);
            } else {
                ProcessPlayerPauseCommand(logger, request, PLAYER_PAUSE_DIRECTIVE, PLAYER_PAUSE_COMMAND,
                                          std::move(directiveValue), bodyBuilder);
            }
            return;
        }

        AddOnlySuggests(logger, request, bodyBuilder);
        return;
    }

    if (request.ClientInfo().IsYaAuto()) {
        if (isWaitingForRouteConfirmation) {
            ProcessNavigatorCancelConfirmation(logger, request, bodyBuilder);
            return;
        }
        ProcessYaAutoStopCommand(request, bodyBuilder);
        return;
    }

    if (request.ClientInfo().IsSmartSpeaker() || request.ClientInfo().IsSearchApp() ||
        request.Interfaces().GetHasMusicQuasarClient())
    {
        TVector<std::pair<TStringBuf, TStringBuf>> directives;

        const auto& musicPlayerState = deviceState.GetMusic().GetPlayer();
        if (musicPlayerState.HasPause() && !musicPlayerState.GetPause() && request.Interfaces().GetSupportsPlayerPauseDirective()) {
            directives.push_back(std::make_pair(PLAYER_PAUSE_DIRECTIVE, PLAYER_PAUSE_COMMAND));
            LOG_INFO(logger) << "Generated directive: player_pause";
        }

        if (request.Interfaces().GetHasAudioClient() &&
            (request.HasExpFlag(::NAlice::NExperiments::EXP_HW_MUSIC_THIN_CLIENT) ||
             request.HasExpFlag(::NAlice::NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE)) &&
            deviceState.HasAudioPlayer())
        {
            const auto& audioPlayerState = deviceState.GetAudioPlayer();
            if (audioPlayerState.GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing) {
                directives.push_back(std::make_pair(CLEAR_QUEUE_DIRECTIVE, PLAYER_PAUSE_COMMAND));
                LOG_INFO(logger) << "Generated directive: clear_queue";
            }
        }

        if (!directives.empty()) {
            ProcessPlayerPauseCommand(logger, request, directives, std::move(directiveValue), bodyBuilder);
            return;
        }
    }

    if (request.ClientInfo().IsSmartSpeaker() && !request.HasExpFlag(EXP_HW_DISABLE_PAUSE_WITHOUT_PLAYER)) {
        // TODO: remove first line of condition after DirectiveSequencer release
        if ((request.HasExpFlag(EXP_HW_AUDIO_PAUSE_ON_COMMON_PAUSE) && request.Interfaces().GetHasAudioClient()) ||
            request.Interfaces().GetHasDirectiveSequencer())
        {
            // Temporary hack to clear client queue, see SK-4623 for the right way
            TVector<std::pair<TStringBuf, TStringBuf>> directives;
            if (request.Interfaces().GetSupportsPlayerPauseDirective()) {
                directives.push_back(std::make_pair(PLAYER_PAUSE_DIRECTIVE, GC_PAUSE_COMMAND));
            }
            directives.push_back(std::make_pair(CLEAR_QUEUE_DIRECTIVE, GC_PAUSE_COMMAND));
            ProcessPlayerPauseCommand(logger, request, directives, std::move(directiveValue), bodyBuilder);
            LOG_INFO(logger) << "Generated two directives: clear_queue+player_pause with GC sub_name";
        } else {
            if (request.Interfaces().GetSupportsPlayerPauseDirective()) {
                ProcessPlayerPauseCommand(logger, request, PLAYER_PAUSE_DIRECTIVE, GC_PAUSE_COMMAND, std::move(directiveValue),
                                          bodyBuilder);
                LOG_INFO(logger) << "Generated directive: player_pause with GC sub_name";
            } else {
                MakeUnsupportedResponse(logger, request, bodyBuilder);
            }
        }
        return;
    }

    if (request.Interfaces().GetSupportsPlayerPauseDirective()) {
        AddOnlySuggests(logger, request, bodyBuilder);
        LOG_INFO(logger) << "Added only suggests";
    } else {
        if (request.Interfaces().GetSupportsShowPromo()) {
            bodyBuilder.AddShowPromoDirective();
            LOG_INFO(logger) << "Adding show promo directive";
        }
        MakeUnsupportedResponse(logger, request, bodyBuilder);
    }
}

} // namespace NAlice::NHollywood::NPause
