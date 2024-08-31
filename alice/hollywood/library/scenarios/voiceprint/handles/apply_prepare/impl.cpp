#include "impl.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TApplyPrepareHandleImpl::TApplyPrepareHandleImpl(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
    , Logger_{Ctx_.Ctx.Logger()}
    , RequestProto_{GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM)}
    , Request_{RequestProto_, Ctx_.ServiceCtx}
    , Nlg_{TNlgWrapper::Create(Ctx_.Ctx.Nlg(), Request_, Ctx_.Rng, Ctx_.UserLang)}
    , ApplyArgs_{Request_.UnpackArguments<TVoiceprintArguments>()}
{}

const TVoiceprintArguments& TApplyPrepareHandleImpl::GetApplyArgs() const {
    return ApplyArgs_;
}

void TApplyPrepareHandleImpl::LogInfoApplyArgs() {
    LOG_INFO(Logger_) << "Apply arguments: " << ApplyArgs_;
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
