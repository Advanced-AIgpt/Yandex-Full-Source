#include "do_not_disturb.h"

#include "common.h"

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NDoNotDisturb {

bool CanProcessDoNotDisturb(const TScenarioRunRequestWrapper& request) {
    return request.Proto().GetBaseRequest().GetInterfaces().GetSupportsDoNotDisturbDirective();
}

TMaybe<TFrame> GetDoNotDisturbFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input, const TScenarioRunRequestWrapper& request)
{
    if (CanProcessDoNotDisturb(request)) {
        if (frame.Defined() && IsIn({DO_NOT_DISTURB_ON_FRAME, DO_NOT_DISTURB_OFF_FRAME}, frame->Name())) {
            return frame;
        }
        if (const auto doNotDisturbOnFrame = input.FindSemanticFrame(DO_NOT_DISTURB_ON_FRAME)) {
            return TFrame::FromProto(*doNotDisturbOnFrame);
        }
        if (const auto doNotDisturbOffFrame = input.FindSemanticFrame(DO_NOT_DISTURB_OFF_FRAME)) {
            return TFrame::FromProto(*doNotDisturbOffFrame);
        }
    }
    return Nothing();
}

void ProcessDoNotDisturbCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame) {
                                
    auto& logger = fastCommandScenarioRunContext.Logger;
    LOG_INFO(logger) << "ProcessDoNotDisturbCommand started...";
    auto& builder = fastCommandScenarioRunContext.RunResponseBuilder;
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
                                
    if (frame.Name() == DO_NOT_DISTURB_ON_FRAME)
        bodyBuilder.AddDoNotDisturbOnDirective();
    else if (frame.Name() == DO_NOT_DISTURB_OFF_FRAME)
        bodyBuilder.AddDoNotDisturbOffDirective();    
}

} // namespace NAlice::NHollywood::NDoNotDisturb
