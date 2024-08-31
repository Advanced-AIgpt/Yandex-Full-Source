#include "impl.h"
#include "main.h"

namespace NAlice::NHollywood::NVoiceprint {

namespace NImpl {

void TApplyPrepareHandleImpl::Do() {
    LogInfoApplyArgs();

    if (HandleEnroll()) {
        return;
    }

    if (HandleRemove()) {
        return;
    }

    if (HandleSetMyName()) {
        return;
    }

    // all voiceprint apply cases expected to be handled by this line
    Y_UNREACHABLE();
}

} // namespace NImpl

void TApplyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    NImpl::TApplyPrepareHandleImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NVoiceprint
