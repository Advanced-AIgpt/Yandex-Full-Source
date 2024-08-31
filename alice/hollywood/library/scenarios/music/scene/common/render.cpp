#include "render.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/framework/core/codegen/gen_server_directives.pb.h>

#include <util/generic/cast.h>

using NAlice::NScenarios::TDirective;
using NAlice::NScenarios::TServerDirective;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

void AddDirective(TRender& render, NScenarios::TDirective&& directive) {
    auto& dirs = render.Directives();

    switch (directive.GetDirectiveCase()) {
    case TDirective::kSetGlagolMetadataDirective:
        dirs.AddSetGlagolMetadataDirective(std::move(*directive.MutableSetGlagolMetadataDirective()));
        break;
    case TDirective::kAudioRewindDirective:
        dirs.AddAudioRewindDirective(std::move(*directive.MutableAudioRewindDirective()));
        break;
    case TDirective::kShowViewDirective:
        dirs.AddShowViewDirective(std::move(*directive.MutableShowViewDirective()));
        break;
    case TDirective::kAudioPlayDirective:
        dirs.AddAudioPlayDirective(std::move(*directive.MutableAudioPlayDirective()));
        break;
    case TDirective::kMultiroomSemanticFrameDirective:
        dirs.AddMultiroomSemanticFrameDirective(std::move(*directive.MutableMultiroomSemanticFrameDirective()));
        break;
    case TDirective::kClearQueueDirective:
        dirs.AddClearQueueDirective(std::move(*directive.MutableClearQueueDirective()));
        break;
    case TDirective::kSetFixedEqualizerBandsDirective:
        dirs.AddSetFixedEqualizerBandsDirective(std::move(*directive.MutableSetFixedEqualizerBandsDirective()));
        break;
    case TDirective::kSetAdjustableEqualizerBandsDirective:
        dirs.AddSetAdjustableEqualizerBandsDirective(std::move(*directive.MutableSetAdjustableEqualizerBandsDirective()));
        break;
    case TDirective::kDrawLedScreenDirective:
        dirs.AddDrawLedScreenDirective(std::move(*directive.MutableDrawLedScreenDirective()));
        break;
    case TDirective::kTtsPlayPlaceholderDirective:
        dirs.AddTtsPlayPlaceholderDirective(std::move(*directive.MutableTtsPlayPlaceholderDirective()));
        break;
    default:
        HW_ERROR("Can't render directive with case " << ToUnderlying(directive.GetDirectiveCase()));
    }
}

void AddServerDirective(TRender& render, NScenarios::TServerDirective&& serverDirective) {
    auto& serverDirs = render.ServerDirectives();

    switch (serverDirective.GetDirectiveCase()) {
    case TServerDirective::kPushTypedSemanticFrameDirective:
        serverDirs.AddPushTypedSemanticFrameDirective(std::move(*serverDirective.MutablePushTypedSemanticFrameDirective()));
        break;
    default:
        HW_ERROR("Can't render server directive with case " << ToUnderlying(serverDirective.GetDirectiveCase()));
    }
}

} // namespace

void DoCommonRender(const TMusicScenarioRenderArgsCommon& constRenderArgs, TRender& render) {
    auto renderArgs = constRenderArgs; // make a copy

    if (renderArgs.HasNlgData()) {
        const auto& nlgData = renderArgs.GetNlgData();
        render.CreateFromNlg(nlgData.GetTemplate(), nlgData.GetPhrase(), nlgData.GetContext());
    }

    for (auto&& directive : *renderArgs.MutableDirectiveList()) {
        AddDirective(render, std::move(directive));
    }

    for (auto&& serverDirective : *renderArgs.MutableServerDirectiveList()) {
        AddServerDirective(render, std::move(serverDirective));
    }

    if (renderArgs.HasStackEngine()) {
        render.SetStackEngine(std::move(*renderArgs.MutableStackEngine()));
    }
}

TRetResponse CommonRender(const TMusicScenarioRenderArgsCommon& renderArgs, TRender& render) {
    DoCommonRender(renderArgs, render);
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NMusic
