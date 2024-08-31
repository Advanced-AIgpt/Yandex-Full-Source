#include "modifier_prepare_handle.h"
#include <alice/hollywood/library/modifiers/registry/modifier_registry.h>

namespace NAlice::NHollywood::NModifiers {

namespace NImpl {

void PrepareImpl(
    const NMegamind::TModifierRequest& request,
    IModifierContext& ctx,
    IExternalSourceRequestCollector& externalSourcesRequestCollector,
    const TVector<TBaseModifierPtr>& modifiers)
{
    for (const auto& modifier : modifiers) {
        if (!modifier->IsEnabled(ctx)) {
            continue;
        }

        LOG_INFO(ctx.Logger()) << "Preparing modifier " << modifier->GetModifierType();
        modifier->Prepare(TModifierPrepareContext{ctx, request.GetModifierBody(), externalSourcesRequestCollector});
    }
}

} // namespace NImpl

void TModifierPrepareHandle::Do(THwServiceContext& ctx) const {
    Y_ENSURE(Modifiers_.Defined(), "Modifiers are not initialized");
    const auto request = ctx.GetProtoOrThrow<NMegamind::TModifierRequest>(MODIFIER_REQUEST_ITEM);
    TRng rng{MultiHash(request.GetBaseRequest().GetRandomSeed(), Name())};
    TModifierContext modifierContext{ctx.Logger(), request.GetFeatures(), request.GetBaseRequest(), rng, ctx.Sensors()};
    TExternalSourceRequestCollector externalSourcesRequestCollector{ctx.ApphostContext()};
    NImpl::PrepareImpl(request, modifierContext, externalSourcesRequestCollector, Modifiers_.GetRef());
}

void TModifierPrepareHandle::InitWithConfig(const THwServicesConfig& config, const TFsPath& resourcesBasePath) {
    Modifiers_ = TModifierRegistry::Get().CreateModifiers(config.GetModifiersConfig(), resourcesBasePath);
}

const TString& TModifierPrepareHandle::Name() const {
    static const TString handleName = "modifiers/prepare";
    return handleName;
}


} // namespace NAlice::NHollywood::NModifiers
