#include "context_wrapper.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

template <typename TRequest, typename TRequestWrapper>
TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::TGeneralConversationContextWrapper(TScenarioHandleContext* ctx)
    : Ctx_(ctx)
    , ScenarioRunRequest_(GetOnlyProtoOrThrow<TRequest>(Ctx_->ServiceCtx, REQUEST_ITEM))
    , RequestWrapper_(ScenarioRunRequest_, ctx->ServiceCtx)
    , NlgWrapper_(TNlgWrapper::Create(Ctx_->Ctx.Nlg(), RequestWrapper_, Ctx_->Rng, Ctx_->UserLang))
    , FastData_(Ctx_->Ctx.GlobalContext().FastData().GetFastData<TGeneralConversationFastData>())
{
}

template <typename TRequest, typename TRequestWrapper>
TScenarioHandleContext* TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::Ctx() {
    return Ctx_;
}

template <typename TRequest, typename TRequestWrapper>
const TGeneralConversationFastData* TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::FastData() {
    return FastData_.get();
}

template <typename TRequest, typename TRequestWrapper>
TRTLogger& TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::Logger() {
    return Ctx_->Ctx.Logger();
}

template <typename TRequest, typename TRequestWrapper>
TNlgWrapper& TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::NlgWrapper() {
    return NlgWrapper_;
}

template <typename TRequest, typename TRequestWrapper>
const TGeneralConversationResources& TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::Resources() const {
    return Ctx_->Ctx.ScenarioResources<TGeneralConversationResources>();
}

template <typename TRequest, typename TRequestWrapper>
const TRequestWrapper& TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::RequestWrapper() const {
    return RequestWrapper_;
}

template <typename TRequest, typename TRequestWrapper>
IRng& TGeneralConversationContextWrapper<TRequest, TRequestWrapper>::Rng() {
    return Ctx_->Rng;
}

template class TGeneralConversationContextWrapper<NScenarios::TScenarioRunRequest, TScenarioRunRequestWrapper>;
template class TGeneralConversationContextWrapper<NScenarios::TScenarioApplyRequest, TScenarioApplyRequestWrapper>;

} // namespace NAlice::NHollywood::NGeneralConversation
