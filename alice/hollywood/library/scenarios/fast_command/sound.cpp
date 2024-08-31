#include "sound.h"

#include "common.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/sound/sound_common.h>
#include <alice/hollywood/library/sound/sound_level_calculation.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/memento/proto/user_configs.pb.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/is_in.h>
#include <util/string/cast.h>


namespace NAlice::NHollywood::NSound {

namespace {

const TVector<TStringBuf> ALL_FRAMES {
    LOUDER_FRAME,
    QUITER_FRAME,
    MUTE_FRAME,
    UNMUTE_FRAME,
    GET_LEVEL_FRAME,
    SET_LEVEL_FRAME
};

const TVector<TStringBuf> NLU_HINT_FRAMES {
    SOUND_LOUDER_NLU_HINT_FRAME_NAME,
    SOUND_QUITER_NLU_HINT_FRAME_NAME,
    SOUND_SET_LEVEL_NLU_HINT_FRAME_NAME
};

void AddSoundEllipsis(TResponseBodyBuilder& bodyBuilder, TStringBuf hintFrameName,
                      TStringBuf semanticFrameName, TStringBuf actionId) {
    NScenarios::TFrameAction frameAction;

    TSemanticFrame semanticFrame;
    semanticFrame.SetName(TString{semanticFrameName});
    *frameAction.MutableCallback() = ToCallback(semanticFrame);

    TFrameNluHint nluHint;
    nluHint.SetFrameName(TString{hintFrameName});
    *frameAction.MutableNluHint() = std::move(nluHint);

    bodyBuilder.AddAction(TString{actionId}, std::move(frameAction));
}

void ProcessNavigatorSound(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TFrame& frame,
                           TRunResponseBuilder& builder) {
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);

    TStringBuilder navigatorIntent;
    navigatorIntent << "yandexnavi://set_setting?";
    TNlgData nlgData{logger, request};
    nlgData.Context["has_alicesdk_player"] = true;

    const auto& frameName = frame.Name();
    if (frameName == MUTE_FRAME) {
        navigatorIntent << "name=soundNotifications&value=Alerts";
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_MUTE_NLG,
                                                       RENDER_RESULT,
                                                       /* buttons = */ {}, nlgData);
        FillAnalyticsInfo(bodyBuilder, SOUND_MUTE_INTENT, NProductScenarios::SOUND_COMMAND);
    } else if (frameName == UNMUTE_FRAME) {
        navigatorIntent << "name=soundNotifications&value=All";
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_UNMUTE_NLG,
                                                       RENDER_RESULT,
                                                       /* buttons = */ {}, nlgData);
        FillAnalyticsInfo(bodyBuilder, SOUND_UNMUTE_INTENT, NProductScenarios::SOUND_COMMAND);
    } else {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                       RENDER_SOUND_NOT_SUPPORTED,
                                                       /* buttons = */ {}, nlgData);
        return;
    }

    NJson::TJsonValue value;
    value["uri"] = navigatorIntent;
    bodyBuilder.AddClientActionDirective(TString{OPEN_URI_DIRECTIVE},
                                         TString{NAVI_SET_SETTINGS_COMMAND}, value);
}

void ProcessYaAutoSound(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TFrame& frame,
                        TRunResponseBuilder& builder) {
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    NJson::TJsonValue value;

    const auto& frameName = frame.Name();
    if (request.ClientInfo().IsOldYaAuto()) {
        if (frameName == LOUDER_FRAME || frameName == SOUND_LOUDER_NLU_HINT_FRAME_NAME) {
            value["application"] = "car";
            value["intent"] = "volume_up";
            bodyBuilder.AddClientActionDirective(TString{CAR_DIRECTIVE}, TString{CAR_VOLUME_UP}, value);
            FillAnalyticsInfo(bodyBuilder, SOUND_LOUDER_INTENT, NProductScenarios::SOUND_COMMAND);
        } else if (frameName == QUITER_FRAME || frameName == SOUND_QUITER_NLU_HINT_FRAME_NAME) {
            value["application"] = "car";
            value["intent"] = "volume_down";
            bodyBuilder.AddClientActionDirective(TString{CAR_DIRECTIVE}, TString{CAR_VOLUME_DOWN}, value);
            FillAnalyticsInfo(bodyBuilder, SOUND_QUITER_INTENT, NProductScenarios::SOUND_COMMAND);
        } else {
            TNlgData nlgData{logger, request};
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                           RENDER_SOUND_NOT_SUPPORTED,
                                                           /* buttons = */ {}, nlgData);
        }
        return;
    }

    if (frameName == LOUDER_FRAME || frameName == SOUND_LOUDER_NLU_HINT_FRAME_NAME) {
        value["uri"] = "yandexauto://sound?action=volume_up";
        AddSoundEllipsis(bodyBuilder, SOUND_LOUDER_NLU_HINT_FRAME_NAME,
                         LOUDER_FRAME, SOUND_LOUDER_NLU_HINT_ACTION_ID);
        FillAnalyticsInfo(bodyBuilder, SOUND_LOUDER_INTENT, NProductScenarios::SOUND_COMMAND);
    } else if (frameName == QUITER_FRAME || frameName == SOUND_QUITER_NLU_HINT_FRAME_NAME) {
        value["uri"] = "yandexauto://sound?action=volume_down";
        AddSoundEllipsis(bodyBuilder, SOUND_QUITER_NLU_HINT_FRAME_NAME,
                         QUITER_FRAME, SOUND_QUITER_NLU_HINT_ACTION_ID);
        FillAnalyticsInfo(bodyBuilder, SOUND_QUITER_INTENT, NProductScenarios::SOUND_COMMAND);
    } else if (frameName == MUTE_FRAME) {
        value["uri"] = "yandexauto://sound?action=mute";
        FillAnalyticsInfo(bodyBuilder, SOUND_MUTE_INTENT, NProductScenarios::SOUND_COMMAND);
    } else if (frameName == UNMUTE_FRAME) {
        value["uri"] = "yandexauto://sound?action=unmute";
        FillAnalyticsInfo(bodyBuilder, SOUND_UNMUTE_INTENT, NProductScenarios::SOUND_COMMAND);
    } else {
        TNlgData nlgData{logger, request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                       RENDER_SOUND_NOT_SUPPORTED,
                                                       /* buttons = */ {}, nlgData);
        return;
    }
    bodyBuilder.AddClientActionDirective(TString{OPEN_URI_DIRECTIVE},
                                         TString{AUTO_SOUND_COMMAND}, value);
}

void SetSoundLevel(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                   TResponseBodyBuilder& bodyBuilder,
                   const TFrame& frame, const TDeviceVolume& deviceSound,
                   const TNlgData& commonNlgData, NJson::TJsonValue directiveValue)
{
    auto config = request.Proto().GetBaseRequest().GetMemento().GetUserConfigs().GetVolumeOnboardingConfig();
    const ui64 usageCounter = config.GetUsageCounter();
    const i64 newSoundLevel = CalculateSoundLevelForSetLevel(frame, deviceSound, usageCounter);

    if (!deviceSound.IsSupported(newSoundLevel)) {
        // If user says "set volume on 11", probably he wants maximum volume
        // So we don't make volume maximal, but warn that Alice has [0;10] (or other) scale
        TNlgData nlgData{logger, request};
        nlgData.Context["error"]["data"]["code"] = "level_out_of_range";
        nlgData.Context["sound_max_level"] = deviceSound.GetMax();

        config.SetUsageCounter(usageCounter + 1);
        bodyBuilder.TryAddMementoUserConfig(ru::yandex::alice::memento::proto::EConfigKey::CK_VOLUME_ONBOARDING, config);

        bodyBuilder.AddRenderedTextAndVoice(SOUND_COMMON_NLG, RENDER_SOUND_ERROR, nlgData);
        AddSoundEllipsis(bodyBuilder, SOUND_SET_LEVEL_NLU_HINT_FRAME_NAME,
                         SET_LEVEL_FRAME, SOUND_SET_LEVEL_NLU_HINT_ACTION_ID);
        return;
    }

    if (deviceSound.GetCurrent() == newSoundLevel) {
        // Sound level directive should be sent, because in is used for multiroom volume synchronization
        directiveValue["new_level"] = newSoundLevel;
        bodyBuilder.AddClientActionDirective(TString{SOUND_SET_LEVEL_DIRECTIVE},
                                             TString{SOUND_SET_LEVEL_COMMAND}, directiveValue);
        TNlgData nlgData{logger, request};
        auto& errorCode = nlgData.Context["error"]["data"]["code"];
        if (deviceSound.IsMaximum()) {
            errorCode = "already_max";
        } else if (deviceSound.IsMinimum()) {
            errorCode = "already_min";
        } else {
            errorCode = "already_set";
        }
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                       RENDER_SOUND_ERROR,
                                                       /* buttons = */ {}, nlgData);
        AddSoundEllipsis(bodyBuilder, SOUND_SET_LEVEL_NLU_HINT_FRAME_NAME,
                         SET_LEVEL_FRAME, SOUND_SET_LEVEL_NLU_HINT_ACTION_ID);
        return;
    }

    directiveValue["new_level"] = newSoundLevel;
    bodyBuilder.AddClientActionDirective(TString{SOUND_SET_LEVEL_DIRECTIVE},
                                         TString{SOUND_SET_LEVEL_COMMAND}, directiveValue);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                   RENDER_RESULT,
                                                   /* buttons = */ {}, commonNlgData);
}

void MakeRelativeSoundChange(TResponseBodyBuilder& bodyBuilder,
                             const TNlgData& commonNlgData, NJson::TJsonValue directiveValue, bool isLouder) {
    if (isLouder) {
        bodyBuilder.AddClientActionDirective(TString{SOUND_LOUDER_DIRECTIVE}, TString{SOUND_LOUDER_COMMAND},
                                             std::move(directiveValue));
    } else {
        bodyBuilder.AddClientActionDirective(TString{SOUND_QUITER_DIRECTIVE}, TString{SOUND_QUITER_COMMAND},
                                             std::move(directiveValue));
    }

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG, RENDER_RESULT, /* buttons = */ {}, commonNlgData);

    if (isLouder) {
        AddSoundEllipsis(bodyBuilder, SOUND_LOUDER_NLU_HINT_FRAME_NAME,
                         LOUDER_FRAME, SOUND_LOUDER_NLU_HINT_ACTION_ID);
    } else {
        AddSoundEllipsis(bodyBuilder, SOUND_QUITER_NLU_HINT_FRAME_NAME,
                         QUITER_FRAME, SOUND_QUITER_NLU_HINT_ACTION_ID);
    }
}

void MakeDirectSoundChange(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                           TResponseBodyBuilder& bodyBuilder,
                           const i64 newSoundLevel, const TDeviceVolume& deviceSound,
                           const TNlgData& commonNlgData, NJson::TJsonValue directiveValue, bool isLouder)
{
    const bool alreadyMin = deviceSound.GetCurrent() <= deviceSound.GetMin() && newSoundLevel <= deviceSound.GetMin();
    const bool alreadyMax = deviceSound.GetCurrent() >= deviceSound.GetMax() && newSoundLevel >= deviceSound.GetMax();

    // Sound level directive is always to be sent, because it is used for multiroom volume synchronization
    directiveValue["new_level"] = newSoundLevel;
    bodyBuilder.AddClientActionDirective(TString{SOUND_SET_LEVEL_DIRECTIVE},
                                         TString{SOUND_SET_LEVEL_COMMAND}, directiveValue);

    if (alreadyMin || alreadyMax) {
        TNlgData nlgData{logger, request};
        if (alreadyMin) {
            nlgData.Context["error"]["data"]["code"] = "already_min";
        } else {
            nlgData.Context["error"]["data"]["code"] = "already_max";
        }
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                       RENDER_SOUND_ERROR,
                                                       /* buttons = */ {}, nlgData);
    } else {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                       RENDER_RESULT,
                                                       /* buttons = */ {}, commonNlgData);
    }

    if (isLouder) {
        AddSoundEllipsis(bodyBuilder, SOUND_LOUDER_NLU_HINT_FRAME_NAME,
                         LOUDER_FRAME, SOUND_LOUDER_NLU_HINT_ACTION_ID);
    } else {
        AddSoundEllipsis(bodyBuilder, SOUND_QUITER_NLU_HINT_FRAME_NAME,
                         QUITER_FRAME, SOUND_QUITER_NLU_HINT_ACTION_ID);
    }
}

void AddAbsoluteNotSupportedText(TResponseBodyBuilder& bodyBuilder, const TNlgData& nlgData) {
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_SET_LEVEL_NLG,
                                                    RENDER_ABSOLUTE_SET_LEVEL_ERROR,
                                                    /* buttons = */ {}, nlgData);
}

void MakeSoundLouderOrQuiter(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                             TResponseBodyBuilder& bodyBuilder,
                             const TFrame& frame, const TDeviceVolume& deviceSound,
                             const TNlgData& commonNlgData, NJson::TJsonValue directiveValue, bool isLouder)
{
    const auto& newSoundAndIsRelativeRequestPair = CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, isLouder);
    if (request.Interfaces().GetSupportsRelativeVolumeChange()) {
        if (newSoundAndIsRelativeRequestPair.second) {
            MakeRelativeSoundChange(bodyBuilder, commonNlgData, std::move(directiveValue), isLouder);
        } else if (request.Interfaces().GetSupportsAbsoluteVolumeChange()){
            MakeDirectSoundChange(logger, request, bodyBuilder, newSoundAndIsRelativeRequestPair.first, deviceSound, commonNlgData, std::move(directiveValue), isLouder);
        } else {
            TNlgData nlgData{logger, request};
            AddAbsoluteNotSupportedText(bodyBuilder, nlgData);
        }
    } else {
        MakeDirectSoundChange(logger, request, bodyBuilder, newSoundAndIsRelativeRequestPair.first, deviceSound, commonNlgData, std::move(directiveValue), isLouder);
    }

}

TNlgData MakeCommonNlgData(TRTLogger& logger, const TScenarioRunRequestWrapper& request) {
    const auto& musicPlayerState = request.Proto().GetBaseRequest().GetDeviceState().GetMusic().GetPlayer();
    TNlgData nlgData{logger, request};
    nlgData.Context["has_alicesdk_player"] = request.Proto().GetBaseRequest().GetInterfaces().GetHasMusicSdkClient();
    nlgData.Context["only_text"] = musicPlayerState.HasPause() && !musicPlayerState.GetPause();
    return nlgData;
}

void RenderSoundNotSupported(
    TRTLogger& logger, const TScenarioRunRequestWrapper& request,
    NAlice::NHollywood::TResponseBodyBuilder& bodyBuilder)
{
    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextAndVoice(SOUND_COMMON_NLG, RENDER_SOUND_NOT_SUPPORTED, nlgData);
}

bool ProcessCommonSound(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TFrame& frame,
                        TRunResponseBuilder& builder) {
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    const auto& deviceState = request.Proto().GetBaseRequest().GetDeviceState();
    const i64 currentSoundLevel = deviceState.GetSoundLevel();

    NJson::TJsonValue directiveValue;
    AddMultiroomSessionIdToDirectiveValue(directiveValue, deviceState);

    const auto& frameName = frame.Name();

    if (!request.Interfaces().GetSupportsAnyPlayer()) {
        RenderSoundNotSupported(logger, request, bodyBuilder);
        if (request.Interfaces().GetSupportsShowPromo()) {
            bodyBuilder.AddShowPromoDirective();
        }
        return true;
    }

    const TDeviceVolume deviceSound = TDeviceVolume::BuildFromState(deviceState);

    const auto commonNlgData = MakeCommonNlgData(logger, request);

    if (frameName == MUTE_FRAME) {
        if (!request.Interfaces().GetSupportsMuteUnmuteVolume()) {
            RenderSoundNotSupported(logger, request, bodyBuilder);
        } else {
            bodyBuilder.AddClientActionDirective(TString{SOUND_MUTE_DIRECTIVE},
                                                 TString{SOUND_MUTE_COMMAND}, {});
            FillAnalyticsInfo(bodyBuilder, SOUND_MUTE_INTENT, NProductScenarios::SOUND_COMMAND);
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_MUTE_NLG,
                                                           RENDER_RESULT,
                                                           /* buttons = */ {}, commonNlgData);
        }

        return true;

    } else if (frameName == UNMUTE_FRAME) {
        if (!request.Interfaces().GetSupportsMuteUnmuteVolume()) {
             RenderSoundNotSupported(logger, request, bodyBuilder);
        } else {
            bodyBuilder.AddClientActionDirective(TString{SOUND_UNMUTE_DIRECTIVE},
                                                 TString{SOUND_UNMUTE_COMMAND}, {});
            FillAnalyticsInfo(bodyBuilder, SOUND_UNMUTE_INTENT, NProductScenarios::SOUND_COMMAND);
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_UNMUTE_NLG,
                                                           RENDER_RESULT,
                                                           /* buttons = */ {}, commonNlgData);
        }

        return true;

    } else if (frameName == LOUDER_FRAME || frameName == SOUND_LOUDER_NLU_HINT_FRAME_NAME) {
        if (!request.Interfaces().GetSupportsRelativeVolumeChange() && !request.Interfaces().GetSupportsAbsoluteVolumeChange()) {
            RenderSoundNotSupported(logger, request, bodyBuilder);
        } else {
            FillAnalyticsInfo(bodyBuilder, SOUND_LOUDER_INTENT, NProductScenarios::SOUND_COMMAND);

            MakeSoundLouderOrQuiter(logger, request, bodyBuilder, frame, deviceSound,
                                    commonNlgData, std::move(directiveValue), /* isLouder = */ true);
        }

        return true;

    } else if (frameName == QUITER_FRAME || frameName == SOUND_QUITER_NLU_HINT_FRAME_NAME) {
        if (!request.Interfaces().GetSupportsRelativeVolumeChange() && !request.Interfaces().GetSupportsAbsoluteVolumeChange()) {
            RenderSoundNotSupported(logger, request, bodyBuilder);
        } else {
            FillAnalyticsInfo(bodyBuilder, SOUND_QUITER_INTENT, NProductScenarios::SOUND_COMMAND);
            MakeSoundLouderOrQuiter(logger, request, bodyBuilder, frame, deviceSound,
                                    commonNlgData, std::move(directiveValue), /* isLouder = */ false);
        }

        return true;

    } else if (frameName == SET_LEVEL_FRAME || frameName == SOUND_SET_LEVEL_NLU_HINT_FRAME_NAME) {
        FillAnalyticsInfo(bodyBuilder, SOUND_SET_INTENT, NProductScenarios::SOUND_COMMAND);

        if (request.Interfaces().GetSupportsAbsoluteVolumeChange()) {
            SetSoundLevel(logger, request, bodyBuilder, frame, deviceSound,
                          commonNlgData, std::move(directiveValue));
        } else if (request.Interfaces().GetSupportsRelativeVolumeChange()) {
            TNlgData nlgData{logger, request};
            AddAbsoluteNotSupportedText(bodyBuilder, nlgData);
        } else {
            // Both absolute and relative volume changing is not supported on the device
            RenderSoundNotSupported(logger, request, bodyBuilder);
        }

        return true;
    } else if (frameName == GET_LEVEL_FRAME) {
        if (!deviceState.HasSoundLevel()) {
            RenderSoundNotSupported(logger, request, bodyBuilder);
        } else {
            FillAnalyticsInfo(bodyBuilder, SOUND_GET_INTENT, NProductScenarios::SOUND_COMMAND);
            TNlgData nlgData{logger, request};
            nlgData.Context["form"]["level"] = currentSoundLevel;
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(TString{SOUND_GET_LEVEL_DIRECTIVE},
                                                           RENDER_RESULT,
                                                           /* buttons = */ {}, nlgData);
            bodyBuilder.SetShouldListen(true);
            AddSoundEllipsis(bodyBuilder, SOUND_SET_LEVEL_NLU_HINT_FRAME_NAME,
                             SET_LEVEL_FRAME, SOUND_SET_LEVEL_NLU_HINT_ACTION_ID);
        }
        return true;
    }

    return false;
}

void ProcessCommandToOtherDevice(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                                 const TFrame& frame, TRunResponseBuilder& builder)
{
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    const auto commonNlgData = MakeCommonNlgData(logger, request);

    NJson::TJsonValue directiveValue;
    AddRedirectToLocation(directiveValue, frame);

    bool success = false;
    if (frame.Name() == QUITER_FRAME) {
        FillAnalyticsInfo(bodyBuilder, SOUND_QUITER_INTENT, NProductScenarios::SOUND_COMMAND);
        bodyBuilder.AddClientActionDirective(TString{SOUND_QUITER_DIRECTIVE}, TString{SOUND_QUITER_COMMAND},
                                             std::move(directiveValue));
        success = true;
    } else if (frame.Name() == LOUDER_FRAME) {
        FillAnalyticsInfo(bodyBuilder, SOUND_LOUDER_INTENT, NProductScenarios::SOUND_COMMAND);
        bodyBuilder.AddClientActionDirective(TString{SOUND_LOUDER_DIRECTIVE}, TString{SOUND_LOUDER_COMMAND},
                                             std::move(directiveValue));
        success = true;
    } else if (frame.Name() == SET_LEVEL_FRAME) {
        FillAnalyticsInfo(bodyBuilder, SOUND_SET_INTENT, NProductScenarios::SOUND_COMMAND);
        if (const auto soundLevel = CalculateSoundLevelForSetLevelOnOtherDevice(frame)) {
            directiveValue["new_level"] = soundLevel.GetRef();
            bodyBuilder.AddClientActionDirective(TString{SOUND_SET_LEVEL_DIRECTIVE},
                                                 TString{SOUND_SET_LEVEL_COMMAND}, std::move(directiveValue));
            success = true;
        }
    }

    const auto& phraseName = success ? RENDER_RESULT : RENDER_SOUND_NOT_SUPPORTED;
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG, phraseName, /* buttons = */ {}, commonNlgData);
}

} // namespace

TMaybe<TFrame> GetSoundFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input) {
    if (frame.Defined() && IsIn(ALL_FRAMES, frame->Name())) {
        return frame;
    }
    return GetFrame(input, ALL_FRAMES);
}

TMaybe<TFrame> GetNluHintFrame(const TScenarioInputWrapper& input) {
    return GetFrame(input, NLU_HINT_FRAMES);
}

void ProcessSoundRequest(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame) {
    const auto& request = fastCommandScenarioRunContext.Request;
    auto& logger = fastCommandScenarioRunContext.Logger;
    auto& builder = fastCommandScenarioRunContext.RunResponseBuilder;

    if (request.ClientInfo().IsNavigator()) {
        ProcessNavigatorSound(logger, request, frame, builder);
        return;
    }

    if (request.ClientInfo().IsYaAuto()) {
        ProcessYaAutoSound(logger, request, frame, builder);
        return;
    }

    if (request.Interfaces().GetMultiroom() && FrameHasSomeLocationSlot(frame)) {
        ProcessCommandToOtherDevice(logger, request, frame, builder);
        return;
    }

    if (ProcessCommonSound(logger, request, frame, builder)) {
        return;
    }

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                   RENDER_SOUND_NOT_SUPPORTED,
                                                   /* buttons = */ {}, nlgData);
    builder.SetIrrelevant();
}

} // namespace NAlice::NHollywood::NSound
