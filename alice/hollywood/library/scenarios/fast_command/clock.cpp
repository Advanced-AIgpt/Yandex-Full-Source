#include "clock.h"
#include "common.h"

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/scled_animations/scled_animations_builder.h>
#include <alice/library/scled_animations/scled_animations_directive_hw.h>
#include <alice/library/versioning/versioning.h>
#include <alice/megamind/protos/common/device_state.pb.h>

#include <library/cpp/timezone_conversion/convert.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NClock {

namespace {

constexpr TStringBuf TURN_ON_CLOCK_FRAME_NAME = "alice.clock_face_control.turn_on";
constexpr TStringBuf TURN_OFF_CLOCK_FRAME_NAME = "alice.clock_face_control.turn_off";

const TString CANT_DELAYED_TURN_ON_CLOCK_INTENT = "alice.clock_face_control.delayed_turn_on_cant";
const TString CANT_DELAYED_TURN_OFF_CLOCK_INTENT = "alice.clock_face_control.delayed_turn_off_cant";

constexpr TStringBuf TIME_PREPOSITION_SLOT = "time_preposition";
constexpr TStringBuf TIME_SLOT = "time";
constexpr TStringBuf DATE_SLOT = "date";
constexpr TStringBuf DAY_PART_SLOT = "day_part";

constexpr TStringBuf NO_DELAYED_ACTION_REQUST_SLOT = "no_delayed_action_request_slot";

const THashMap<TStringBuf, TStringBuf> CLOCK_FRAME_NAMES_TO_EXPS = {
    {TURN_ON_CLOCK_FRAME_NAME, EXP_CLOCK_FACE_CONTROL_TURN_ON},
    {TURN_OFF_CLOCK_FRAME_NAME, EXP_CLOCK_FACE_CONTROL_TURN_OFF},
};

const TVector<TStringBuf> DELAYED_ACTION_REQUEST_SLOTS = {
    TIME_PREPOSITION_SLOT, TIME_SLOT, DATE_SLOT, DAY_PART_SLOT,
};

const THashSet<TStringBuf> CLOCK_FRAME_NAMES = {
    TURN_ON_CLOCK_FRAME_NAME,
    TURN_OFF_CLOCK_FRAME_NAME,
};

const TString GET_TIME_GIF_DEFAULT_SUBVERSION = "2";
constexpr TStringBuf GET_TIME_GIF_VERSION = "1";
constexpr TStringBuf GIF_PATH_SEP = "/";
constexpr TStringBuf GIF_URI_PREFIX = "https://static-alice.s3.yandex.net/led-production/";

TPtrWrapper<TSlot> FindDelayedActionRequestSlot(const TFrame& frame) {
    for (auto slotName : DELAYED_ACTION_REQUEST_SLOTS) {
        if (const auto slot = frame.FindSlot(slotName)) {
            return slot;
        }
    }
    return TPtrWrapper<TSlot>{nullptr, NO_DELAYED_ACTION_REQUST_SLOT};
}

NDatetime::TSimpleTM GetTime(const TClientInfo& clientInfo) {
    TInstant now = TInstant::Seconds(clientInfo.Epoch);
    const NDatetime::TTimeZone tz = NDatetime::GetTimeZone(clientInfo.Timezone);
    return NDatetime::ToCivilTime(now, tz);
}

void AddScledAnimation(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder) {
    const auto time = GetTime(request.ClientInfo());
    const auto pattern = Sprintf("%02u:%02u ", time.Hour, time.Min);

    TScledAnimationBuilder scled;
    scled.AddAnim(pattern, /* bright1*/ 0, /* bright2*/ 255, /* durationMs */ 820, TScledAnimationBuilder::AnimModeFromRight | TScledAnimationBuilder::AnimModeSpeedSmooth);
    scled.AddDraw(pattern, /* brighness= */ 255, /* durationMs= */ 2100);
    NScledAnimation::AddDrawScled(bodyBuilder, scled);
}

void AddLedAnimation(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder) {
    const auto versionFlagFull = TString::Join(NExperiments::EXP_GIF_VERSION, "get_time", ":");
    const auto subversion = request.GetValueFromExpPrefix(versionFlagFull).GetOrElse(GET_TIME_GIF_DEFAULT_SUBVERSION);

    const auto time = GetTime(request.ClientInfo());
    const TString gifName = JoinSeq(
        "-",
        {
            TString{"clock"},
            ToString(time.Hour / 10),
            ToString(time.Hour % 10),
            ToString(time.Min / 10),
            ToString(time.Min % 10),
        }
    );
    const TString gifUri = FormatVersion(
        TString::Join(GIF_URI_PREFIX, "get_time/clocks"),
        TString::Join(gifName, ".gif"),
        GET_TIME_GIF_VERSION,
        subversion,
        /* sep = */ GIF_PATH_SEP
    );

    NScenarios::TDirective directive;
    directive.MutableDrawLedScreenDirective()->AddDrawItem()->SetFrontalLedImage(gifUri);
    bodyBuilder.AddDirective(std::move(directive));
}

void CreateClockResponse(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame) {
    const auto& request = fastCommandScenarioRunContext.Request;
    const auto& clockDisplayState = request.BaseRequestProto().GetDeviceState().GetClockDisplayState();
    auto& builder = fastCommandScenarioRunContext.RunResponseBuilder;
    auto& logger = fastCommandScenarioRunContext.Logger;

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();

    TNlgData nlgData{logger, request};

    if (!request.HasExpFlag(EXP_CLOCK_FACE_CONTROL_DISABLE_TIME_SLOT_ANTITRIGGER)) {
        if (const auto slot = FindDelayedActionRequestSlot(frame)) {
            LOG_INFO(logger) << "Will not add show_clock or hide_clock directive, reason: time slot found: "
                             "{" << slot->Name << " : " << slot->Value.AsString() << "}";

            if (request.HasExpFlag(EXP_CLOCK_FACE_CONTROL_UNSUPPORTED_OPERATION_NLG_RESPONSE)) {
                if (frame.Name() == TURN_OFF_CLOCK_FRAME_NAME) {
                    bodyBuilder.AddRenderedTextWithButtonsAndVoice(CLOCK_FACE_NLG,
                                                                   RENDER_CLOCK_FACE_TURN_OFF_UNSUPPORTED_OPERATION,
                                                                   /* buttons = */ {}, nlgData);
                    analyticsInfoBuilder.SetIntentName(CANT_DELAYED_TURN_OFF_CLOCK_INTENT);
                } else if (frame.Name() == TURN_ON_CLOCK_FRAME_NAME) {
                    bodyBuilder.AddRenderedTextWithButtonsAndVoice(CLOCK_FACE_NLG,
                                                                   RENDER_CLOCK_FACE_TURN_ON_UNSUPPORTED_OPERATION,
                                                                   /* buttons = */ {}, nlgData);
                    analyticsInfoBuilder.SetIntentName(CANT_DELAYED_TURN_ON_CLOCK_INTENT);
                }

                analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::LED_CLOCK_COMMANDS);
            }

            return;
        }
    }


    TMaybe<NScenarios::TDirective> directive;
    if (frame.Name() == TURN_OFF_CLOCK_FRAME_NAME) {
        if (clockDisplayState.GetClockEnabled()) {
            directive.ConstructInPlace().MutableHideClockDirective();
        } else {
            // clock is already disabled, write NLG
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(CLOCK_FACE_NLG,
                                                           RENDER_CLOCK_FACE_ALREADY_TURNED_OFF,
                                                           /* buttons = */ {}, nlgData);
        }
    } else if (frame.Name() == TURN_ON_CLOCK_FRAME_NAME) {
        directive.ConstructInPlace().MutableShowClockDirective();

        bool needAddTtsPlayPlaceholder = false;
        if (request.Interfaces().GetHasScledDisplay()) {
            AddScledAnimation(request, bodyBuilder);
            needAddTtsPlayPlaceholder = true;
        }
        if (request.Interfaces().GetHasLedDisplay()) {
            AddLedAnimation(request, bodyBuilder);
            needAddTtsPlayPlaceholder = true;
        }
        if (needAddTtsPlayPlaceholder) {
            bodyBuilder.AddTtsPlayPlaceholderDirective();
        }
    }
    if (directive) {
        bodyBuilder.AddDirective(std::move(*directive));
    }

    analyticsInfoBuilder.SetIntentName(frame.Name());
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::LED_CLOCK_COMMANDS);
}

bool HasClockFaceExp(const TScenarioRunRequestWrapper& request, const TString& frameName) {
    return CLOCK_FRAME_NAMES_TO_EXPS.contains(frameName) && request.HasExpFlag(CLOCK_FRAME_NAMES_TO_EXPS.at(frameName));
}

} // namespace

bool TryCreateClockResponse(TFastCommandScenarioRunContext& fastCommandScenarioRunContext) {
    const auto& request = fastCommandScenarioRunContext.Request;
    if (!request.Interfaces().GetHasClockDisplay()) {
        return false;
    }

    if (const auto frame = GetCallbackFrame(request.Input().GetCallback())) {
        if (CLOCK_FRAME_NAMES.contains(frame->Name()) && HasClockFaceExp(request, frame->Name())) {
            CreateClockResponse(fastCommandScenarioRunContext, *frame);
            return true;
        }
    }

    for (const auto frameName : CLOCK_FRAME_NAMES) {
        if (const auto frameProto = request.Input().FindSemanticFrame(frameName)) {
            if (HasClockFaceExp(request, TString(frameName))) {
                CreateClockResponse(fastCommandScenarioRunContext, TFrame::FromProto(*frameProto));
                return true;
            }
        }
    }

    return false;
}

} // namespace NAlice::NHollywood::NClock
