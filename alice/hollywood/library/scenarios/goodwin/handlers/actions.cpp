#include "actions.h"

namespace NAlice {
namespace NFrameFiller {
namespace NGoodwin {

NScenarios::TFrameAction ToFrameAction(const TAction& action, const TString& actionName) {
    NScenarios::TFrameAction frameAction;
    *frameAction.MutableNluHint()->MutableInstances() = action.GetNluHint().GetInstances();
    *frameAction.MutableNluHint()->MutableNegatives() = action.GetNluHint().GetNegatives();
    frameAction.MutableNluHint()->SetFrameName(actionName);
    if (action.GetDirectives().size() == 0) {
        return frameAction;
    }
    if (action.GetDirectives().size() == 1 &&
        action.GetDirectives(0).GetDirectiveCase() == NScenarios::TDirective::DirectiveCase::kCallbackDirective
    ) {
        *frameAction.MutableCallback() = action.GetDirectives(0).GetCallbackDirective();
    } else {
        *frameAction.MutableDirectives()->MutableList() = action.GetDirectives();
    }
    return frameAction;
}

} // namespace NGoodwin
} // namespace NFrameFiller
} // namespace NAlice
