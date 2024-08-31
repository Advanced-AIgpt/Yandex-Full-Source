#include "music_player_commands.h"

#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf PLAYER_CONTINUE_FORM_NAME = "personal_assistant.scenarios.player_continue";
constexpr TStringBuf PLAYER_CONTINUE_FRAME_NAME = "personal_assistant.scenarios.player.continue";
constexpr TStringBuf PLAYER_SHUFFLE_FORM_NAME = "personal_assistant.scenarios.player_shuffle";
constexpr TStringBuf PLAYER_NEXT_FORM_NAME = "personal_assistant.scenarios.player_next_track";
constexpr TStringBuf PLAYER_PREV_FORM_NAME = "personal_assistant.scenarios.player_previous_track";

const TString MUSIC_PLAYER_ONLY_SLOT = "music_player_only";

const TVector<std::pair<TStringBuf, TStringBuf>> FramesWithForms = {
    {
        TStringBuf("personal_assistant.scenarios.player.next_track"),
        PLAYER_NEXT_FORM_NAME
    },
    {
        TStringBuf("personal_assistant.scenarios.player.previous_track"),
        PLAYER_PREV_FORM_NAME
    },
    {
        TStringBuf("personal_assistant.scenarios.player.shuffle"),
        PLAYER_SHUFFLE_FORM_NAME
    },
    {
        PLAYER_CONTINUE_FRAME_NAME,
        PLAYER_CONTINUE_FORM_NAME
    }
};

bool CanPlayMusic(const TScenarioRunRequestWrapper& request) {
    return request.Proto().GetBaseRequest().GetDeviceState().HasMusic() ||
           request.Proto().GetBaseRequest().GetDeviceState().GetBluetooth().HasCurrentlyPlaying();
}

NScenarios::TScenarioRunResponse CreateIrrelevantResponse(
    TNlgWrapper& nlg,
    TRTLogger& logger,
    const TScenarioRunRequestWrapper& request)
{
    // should have been THwFrameworkRunResponseBuilder, but that would make no sense
    TRunResponseBuilder response(&nlg, ConstructBodyRenderer(request));
    response.SetIrrelevant();

    auto& bodyBuilder = response.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    nlgData.Context["error"]["data"]["code"] = "unsupported_operation";
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "render_error__musicerror",
                                                   /* buttons = */ {}, nlgData);
    return *std::move(response).BuildResponse();
}

} // namespace

TMaybe<TStringBuf> GetMusicPlayerFrame(const TScenarioRunRequestWrapper& request) {
    for (const auto& [frame, form] : FramesWithForms) {
        if (request.Input().FindSemanticFrame(frame)) {
            if ((form == PLAYER_SHUFFLE_FORM_NAME && !request.HasExpFlag(EXP_HW_ENABLE_SHUFFLE_IN_HW_MUSIC)) ||
                (form == PLAYER_CONTINUE_FORM_NAME && !request.HasExpFlag(NExperiments::EXP_ENABLE_CONTINUE_IN_HW_MUSIC)))
            {
                return Nothing();
            }
            return form;
        }
    }
    return Nothing();
}

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse, TString>
HandleMusicPlayerCommand(NAlice::TRTLogger& logger,
                         const TScenarioRunRequestWrapper& request,
                         const NScenarios::TRequestMeta& meta,
                         TNlgWrapper& nlg,
                         const TStringBuf playerFrame,
                         const NJson::TJsonValue& appHostParams) {
    if (!CanPlayMusic(request)) {
        return CreateIrrelevantResponse(nlg, logger, request);
    }

    TFrame frame{TString{playerFrame}};
    frame.AddSlot(TSlot{MUSIC_PLAYER_ONLY_SLOT, "flag", {}});
    if (playerFrame == PLAYER_SHUFFLE_FORM_NAME) {
        return PrepareBassVinsRequest(logger, request, frame, /* sourceTextProvider= */ nullptr,
                                      meta, /* imageSearch= */ false, appHostParams);
    }

    if (playerFrame == PLAYER_NEXT_FORM_NAME || playerFrame == PLAYER_PREV_FORM_NAME)
    {
        return PrepareBassRunRequest(logger, request, frame, /* sourceTextProvider= */ nullptr,
                                     meta, /* imageSearch= */ false, appHostParams);
    }

    if (playerFrame == PLAYER_CONTINUE_FORM_NAME) {
        const auto inputFrame = request.Input().FindSemanticFrame(PLAYER_CONTINUE_FRAME_NAME);
        auto continueFrame = TFrame::FromProto(*inputFrame);
        continueFrame.SetName(TString{PLAYER_CONTINUE_FORM_NAME});
        continueFrame.AddSlot(TSlot{MUSIC_PLAYER_ONLY_SLOT, "flag", {}});
        return PrepareBassVinsRequest(logger, request, continueFrame, /* sourceTextProvider= */ nullptr,
                                      meta, /* imageSearch= */ false, appHostParams);
    }

    return CreateIrrelevantResponse(nlg, logger, request);
}

} // NAlice::NHollywood::NMusic
