#include "cloud_ui_modifier.h"

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

const TString MODIFIER_TYPE = "cloud_ui";
constexpr TStringBuf EXP_DISABLE_CLOUD_UI_MODIFIER = "mm_disable_cloud_ui_modifier";

enum class EUriType {
    Http,
    MusicSdk,
    Viewport,
};

// XXX(sparkle): move it to NLG renderer?
const THashMap<EUriType, TStringBuf> URI_TYPE_TO_TEXT_MAPPING = {
    {EUriType::Http, "Открываю"},
    {EUriType::MusicSdk, "Включаю"},
    {EUriType::Viewport, "Вот что я нашла"},
};

bool HasFillCloudUiDirective(const TResponseBodyBuilder& responseBody) {
    const auto& directives = responseBody.GetModifierBody().GetLayout().GetDirectives();
    return AnyOf(directives, [](const auto& d) {
        return d.HasFillCloudUiDirective();
    });
}

TMaybe<EUriType> TryCalculateUriType(const TStringBuf uri) {
    if (uri.StartsWith("http://") || uri.StartsWith("https://")) {
        return EUriType::Http;
    }
    if (uri.StartsWith("musicsdk://")) {
        return EUriType::MusicSdk;
    }
    if (uri.StartsWith("viewport://")) {
        return EUriType::Viewport;
    }
    return Nothing();
}

TMaybe<EUriType> TryCalculateUriType(const NScenarios::TDirective& directive) {
    if (!directive.HasOpenUriDirective()) {
        return Nothing();
    }
    return TryCalculateUriType(directive.GetOpenUriDirective().GetUri());
}

TMaybe<EUriType> TryCalculateUriType(const TResponseBodyBuilder& responseBody) {
    for (const auto& directive : responseBody.GetModifierBody().GetLayout().GetDirectives()) {
        if (auto uriType = TryCalculateUriType(directive)) {
            return std::move(uriType);
        }
    }
    return Nothing();
}

void AddFillCloudUiDirective(TResponseBodyBuilder& responseBody, const TStringBuf text) {
    NScenarios::TDirective directive;
    auto& fillCloudUiDirective = *directive.MutableFillCloudUiDirective();
    fillCloudUiDirective.SetText(text.data(), text.size());
    responseBody.AddDirective(std::move(directive));
}

} // namespace

TCloudUiModifier::TCloudUiModifier()
    : TBaseModifier{MODIFIER_TYPE}
{}

TApplyResult TCloudUiModifier::TryApply(TModifierApplyContext applyCtx) const {
    // the device doesn't support CloudUIFilling at all
    if (!applyCtx.ModifierContext.GetBaseRequest().GetInterfaces().GetSupportsCloudUiFilling()) {
        return TNonApply{TNonApply::EType::NotApplicable};
    }

    // the flag for modifier disabling is present
    if (applyCtx.ModifierContext.HasExpFlag(EXP_DISABLE_CLOUD_UI_MODIFIER)) {
        return TNonApply{TNonApply::EType::DisabledByFlag};
    }

    // the directive is already present
    if (HasFillCloudUiDirective(applyCtx.ResponseBody)) {
        return TNonApply{TNonApply::EType::NotApplicable};
    }

    const auto uriType = TryCalculateUriType(applyCtx.ResponseBody);

    // uri (from TOpenUriDirective) not found
    if (!uriType.Defined()) {
        return TNonApply{TNonApply::EType::NotApplicable};
    }

    // add directive
    const auto* text = URI_TYPE_TO_TEXT_MAPPING.FindPtr(*uriType);
    Y_ENSURE(text);
    AddFillCloudUiDirective(applyCtx.ResponseBody, *text);

    return Nothing(); // success apply
}

} // namespace NAlice::NHollywood::NModifiers
