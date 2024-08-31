#include "media_session.h"
#include "common.h"

#include <util/generic/yexception.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMediaSession {

TMaybe<TFrame> GetMediaSessionFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input)
{
    if (frame.Defined() && IsIn({MEDIA_SESSION_PLAY_FRAME, MEDIA_SESSION_PAUSE_FRAME}, frame->Name())) {
        return frame;
    }
    if (const auto mediaSessionPlayFrame = input.FindSemanticFrame(MEDIA_SESSION_PLAY_FRAME)) {
        return TFrame::FromProto(*mediaSessionPlayFrame);
    }
    if (const auto mediaSessionPauseFrame = input.FindSemanticFrame(MEDIA_SESSION_PAUSE_FRAME)) {
        return TFrame::FromProto(*mediaSessionPauseFrame);
    }
    return Nothing();
}

void ProcessMediaSessionCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame) {
                                
    auto& logger = fastCommandScenarioRunContext.Logger;
    LOG_INFO(logger) << "ProcessMediaSessionCommand started...";

    auto& builder = fastCommandScenarioRunContext.RunResponseBuilder;
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);

    const auto mediaSessionIdSlot = frame.FindSlot("media_session_id");
    if (mediaSessionIdSlot.IsValid()) {
        const auto& mediaSessionId = mediaSessionIdSlot->Value.AsString();
        if (frame.Name() == MEDIA_SESSION_PLAY_FRAME)
            bodyBuilder.AddWebViewMediaSessionPlayDirective(mediaSessionId);
        else if (frame.Name() == MEDIA_SESSION_PAUSE_FRAME)
            bodyBuilder.AddWebViewMediaSessionPauseDirective(mediaSessionId); 
    } else {
        ythrow yexception() << "Invalid data in semantic frame " << frame.Name() << ".";
    }       
}

} // namespace NAlice::NHollywood::NMediaSession
