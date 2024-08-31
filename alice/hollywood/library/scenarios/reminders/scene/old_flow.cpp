#include "old_flow.h"
#include <alice/hollywood/library/scenarios/reminders/entry_point.h>

namespace NAlice::NHollywoodFw::NReminders {

bool TRemindersOldFlowScene::IsRequestSupported(const TRunRequest& runRequest) {
    using namespace NAlice::NHollywood::NReminders;

    if (runRequest.GetApphostInfo().NodeType != ENodeType::Run) {
        return false;
    }

    if (const auto callback = runRequest.Input().FindCallback()) {
        if (TRemindersEntryPointHandler::IsCallbackSupported(callback->GetName())) {
            return true;
        }
    }

    for (const TStringBuf frameName : TRemindersEntryPointHandler::GetSupportedFrames()) {
        if (runRequest.Input().HasSemanticFrame(frameName)) {
            return true;
        }
    }

    return false;
}

}  // namespace NAlice::NHollywood::NReminders
