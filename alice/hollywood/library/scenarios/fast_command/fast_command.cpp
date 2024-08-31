#include "fast_command.h"

#include "clock.h"
#include "common.h"
#include "do_not_disturb.h"
#include "frame_redirect.h"
#include "media_play.h"
#include "media_session.h"
#include "multiroom.h"
#include "pause.h"
#include "sound.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/generic/maybe.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

void TFastCommandRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder runResponseBuilder(&nlgWrapper);

    TFastCommandScenarioRunContext fastCommandScenarioRunContext{ctx.Ctx.Logger(), runResponseBuilder, request};
    DoImpl(fastCommandScenarioRunContext);

    ctx.ServiceCtx.AddProtobufItem(*std::move(runResponseBuilder).BuildResponse(), RESPONSE_ITEM);
}

void TFastCommandRunHandle::DoImpl(TFastCommandScenarioRunContext& fastCommandScenarioRunContext) const {
    const auto& request = fastCommandScenarioRunContext.Request;
    const TMaybe<TFrame> frame = GetCallbackFrame(request.Input().GetCallback());

    if (const auto pauseFrame = NPause::GetPauseFrame(frame, request.Input(), request.ClientInfo(), request)) {
        NPause::ProcessFastPauseCommand(fastCommandScenarioRunContext, *pauseFrame);
        return;
    }

    if (NRedirect::TryCreateMultiroomRedirectResponse(fastCommandScenarioRunContext)) {
        return;
    }

    if (NClock::TryCreateClockResponse(fastCommandScenarioRunContext)) {
        return;
    }

    if (const auto& soundFrame = NSound::GetSoundFrame(frame, request.Input()); soundFrame.Defined()) {
        if (const auto& hintFrame = NSound::GetNluHintFrame(request.Input()); hintFrame.Defined()) {
            NSound::ProcessSoundRequest(fastCommandScenarioRunContext, *hintFrame);
        } else {
            NSound::ProcessSoundRequest(fastCommandScenarioRunContext, *soundFrame);
        }
        return;
    }

    if (const auto& multiroomFrame = NMultiroom::GetMultiroomFrame(frame, request.Input()); multiroomFrame.Defined()) {
        NMultiroom::ProcessMultiroomCommand(fastCommandScenarioRunContext, *multiroomFrame);
        return;
    }

    if (NMediaPlay::TryProcessMediaPlayFrame(fastCommandScenarioRunContext)) {
        return;
    }

    if (const auto& doNotDisturbFrame = NDoNotDisturb::GetDoNotDisturbFrame(frame, request.Input(), request);
        doNotDisturbFrame.Defined())
    {
        NDoNotDisturb::ProcessDoNotDisturbCommand(fastCommandScenarioRunContext, *doNotDisturbFrame);
        return;
    }

    if (const auto& mediaSessionFrame = NMediaSession::GetMediaSessionFrame(frame, request.Input());
        mediaSessionFrame.Defined())
    {
        NMediaSession::ProcessMediaSessionCommand(fastCommandScenarioRunContext, *mediaSessionFrame);
        return;
    }


    // Make irrelevant response.
    TNlgData nlgData{fastCommandScenarioRunContext.Logger, request};
    auto& responseBodyBuilder = fastCommandScenarioRunContext.RunResponseBuilder.CreateResponseBodyBuilder();
    responseBodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG, RENDER_SOUND_COMMON_ERROR, /* buttons = */ {}, nlgData);
    fastCommandScenarioRunContext.RunResponseBuilder.SetIrrelevant();
}

REGISTER_SCENARIO("fast_command",
                  AddHandle<TFastCommandRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NFastCommand::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
