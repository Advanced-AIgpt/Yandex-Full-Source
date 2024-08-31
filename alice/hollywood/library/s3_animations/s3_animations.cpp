#include "s3_animations.h"

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <google/protobuf/any.pb.h>

#include <alice/hollywood/library/environment_state/endpoint.h>

#include <util/generic/algorithm.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf BUCKET_URL = "https://quasar.s3.yandex.net";
const TString DRAW_ANIMATION_DIRECTIVE_NAME= "draw_animation_directive";

bool SupportsS3Animations(const TAnimationCapability& capability) {
    return AnyOf(capability.GetParameters().GetSupportedFormats(), [](const auto format) {
        return format == TAnimationCapability_EFormat_S3Url;
    });
}

} // namespace

bool DeviceSupportsS3Animations(const TEnvironmentState& environmentState, TStringBuf deviceId) {
    const auto* endpoint = FindEndpoint(environmentState, deviceId);
    if (!endpoint) {
        return false;
    }

    TAnimationCapability capability;
    if (!ParseTypedCapability(capability, *endpoint)) {
        return false;
    }

    return SupportsS3Animations(capability);
}

NScenarios::TDirective BuildDrawAnimationDirective(const TStringBuf animationPath,
                                                   TAnimationCapability_TDrawAnimationDirective_ESpeakingAnimationPolicy policy)
{
    NScenarios::TDirective directive;
    auto& drawAnimationDirective = *directive.MutableDrawAnimationDirective();
    drawAnimationDirective.SetName(DRAW_ANIMATION_DIRECTIVE_NAME);
    drawAnimationDirective.SetSpeakingAnimationPolicy(policy);
    auto& s3Directory = *drawAnimationDirective.AddAnimations()->MutableS3Directory();
    s3Directory.SetBucket(BUCKET_URL.data(), BUCKET_URL.size());
    s3Directory.SetPath(animationPath.data(), animationPath.size());
    return directive;
}

} // namespace NAlice::NHollywood
