#include "set_my_name_run.h"

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint {

TSetMyNameRunContext::TSetMyNameRunContext(TVoiceprintHandleContext& voiceprintCtx)
    : TRunStateMachineContextBase<NImpl::TSetMyNameRunStateBase>(voiceprintCtx)
{}

std::unique_ptr<TSetMyNameRunContext> TSetMyNameRunContext::MakeFrom(TVoiceprintHandleContext& voiceprintCtx) {
    auto setMyNameCtx = std::unique_ptr<TSetMyNameRunContext>(new TSetMyNameRunContext(voiceprintCtx));
    setMyNameCtx->State_ = std::make_unique<NImpl::TSetMyNameBiometryDispatchState>(setMyNameCtx.get());
    return std::move(setMyNameCtx);
}

DEFINE_CONTEXT_HANDLE_METHOD(TSetMyNameRunContext, HandleSetMyNameFrame)

} // namespace NAlice::NHollywood::NVoiceprint
