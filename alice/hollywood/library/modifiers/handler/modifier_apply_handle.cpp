#include "modifier_apply_handle.h"

#include <alice/hollywood/library/modifiers/analytics_info/builder.h>
#include <alice/hollywood/library/modifiers/metrics/names.h>
#include <alice/hollywood/library/modifiers/registry/modifier_registry.h>
#include <alice/hollywood/library/modifiers/response_body_builder/builder.h>

#include <alice/library/util/rng.h>
#include <alice/megamind/protos/modifiers/modifier_request.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>

namespace NAlice::NHollywood::NModifiers {

inline constexpr TStringBuf MODIFIER_REQUEST_ITEM = "mm_modifier_request";
inline constexpr TStringBuf MODIFIER_RESPONSE_ITEM = "mm_modifier_response";

using TModifierResponse = NMegamind::TModifierResponse;

namespace {

TModifierResponse BuildResponse(TResponseBodyBuilder&& bodyBuilder, TModifierAnalyticsInfoBuilder&& analyticsInfo) {
    TModifierResponse response{};
    *response.MutableModifierBody() = std::move(std::move(bodyBuilder).MoveProto());
    *response.MutableAnalytics() = std::move(analyticsInfo).MoveProto();
    return response;
}

} // namespace

namespace NImpl {

TModifierResponse ApplyImpl(const NAppHost::IServiceContext& apphostContext, const NMegamind::TModifierRequest& request, IModifierContext& ctx, const TVector<TBaseModifierPtr>& modifiers) {
    TResponseBodyBuilder bodyBuilder{request.GetModifierBody()};
    TModifierAnalyticsInfoBuilder analyticsInfoBuilder{};

    for (const auto& modifier : modifiers) {
        if (!modifier->IsEnabled(ctx)) {
            continue;
        }

        const auto& modifierType = modifier->GetModifierType();

        const auto applyResult = modifier->TryApply(TModifierApplyContext{
            ctx,
            bodyBuilder,
            analyticsInfoBuilder,
            TExternalSourcesResponseRetriever(apphostContext),
        });
        if (!applyResult.Defined()) {
            ctx.Sensors().IncRate(NMetrics::LabelsForModifiersWps(modifierType));
            LOG_INFO(ctx.Logger()) << "Applied " << modifierType << " modifier successfully";
        } else {
            LOG_INFO(ctx.Logger()) << "Didn't apply " << modifierType << " modifier, reason: " << applyResult.Get()->Reason();
        }
    }
    return BuildResponse(std::move(bodyBuilder), std::move(analyticsInfoBuilder));
}

} // namespace NImpl

void TModifierApplyHandle::Do(THwServiceContext& ctx) const {
    Y_ENSURE(Modifiers_.Defined(), "Modifiers are not initialized");
    const auto request = ctx.GetProtoOrThrow<NMegamind::TModifierRequest>(MODIFIER_REQUEST_ITEM);
    TRng rng{MultiHash(request.GetBaseRequest().GetRandomSeed(), Name())};
    TModifierContext modifierContext{ctx.Logger(), request.GetFeatures(), request.GetBaseRequest(), rng, ctx.Sensors()};
    const auto modifierResponse = NImpl::ApplyImpl(ctx.ApphostContext(), request, modifierContext, Modifiers_.GetRef());
    ctx.AddProtobufItemToApphostContext(modifierResponse, MODIFIER_RESPONSE_ITEM);
}

void TModifierApplyHandle::InitWithConfig(const THwServicesConfig& config, const TFsPath& resourcesBasePath) {
    Modifiers_ = TModifierRegistry::Get().CreateModifiers(config.GetModifiersConfig(), resourcesBasePath);
}

const TString& TModifierApplyHandle::Name() const {
    static const TString handleName = "modifiers/apply";
    return handleName;
}

} // namespace NAlice::NHollywood::NModifiers
