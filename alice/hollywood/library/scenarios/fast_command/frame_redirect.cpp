#include "frame_redirect.h"
#include "common.h"

#include <alice/hollywood/library/frame_redirect/frame_redirect.h>
#include <alice/hollywood/library/multiroom/multiroom.h>

namespace NAlice::NHollywood::NRedirect {

namespace {

TVector<std::pair<TStringBuf, TStringBuf>> REDIRECTABLE_SEMANTIC_FRAMES = {
    {"personal_assistant.scenarios.player.pause", "stop"},
};

} // namespace

bool TryCreateMultiroomRedirectResponse(TFastCommandScenarioRunContext& fastCommandScenarioRunContext) {
    const auto& request = fastCommandScenarioRunContext.Request;
    // check if device system doesn't support multiroom redirect
    if (!request.Interfaces().GetMultiroomAudioClient()) {
        return false;
    }

    // collect redirect types
    TFrameRedirectTypeFlags flags;
    if (request.HasExpFlag(EXP_COMMANDS_MULTIROOM_REDIRECT)) {
        flags |= EFrameRedirectType::Server;
    }
    if (request.HasExpFlag(EXP_COMMANDS_MULTIROOM_CLIENT_REDIRECT)) {
        flags |= EFrameRedirectType::Client;
    }
    if (!flags) {
        return false;
    }

    // try get master device id (fails if device is not slave)
    TMultiroom multiroom{request, NAlice::TRTLogger::NullLogger()};
    TStringBuf masterDeviceId;
    if (!multiroom.IsDeviceSlave(masterDeviceId)) {
        return false;
    }

    for (const auto& [frameName, productScenarioName] : REDIRECTABLE_SEMANTIC_FRAMES) {
        if (const auto frame = request.Input().FindSemanticFrame(frameName)) {
            // check if multiroom is not active and frame doesn't contain activation slots
            if (!multiroom.IsActiveWithFrame(frame)) {
                continue;
            }
            LOG_INFO(fastCommandScenarioRunContext.Logger) << "Will redirect frame \"" << frameName << "\" to device \""
                << masterDeviceId << "\"";

            auto frameRedirectResponse = TFrameRedirectBuilder(*frame, GetUid(request), masterDeviceId, flags)
                .SetProductScenarioName(productScenarioName)
                .BuildResponse();
            auto& builder = fastCommandScenarioRunContext.RunResponseBuilder;
            if (frameRedirectResponse.Directive) {
                NScenarios::TDirective directive;
                *directive.MutableMultiroomSemanticFrameDirective() = std::move(*frameRedirectResponse.Directive);
                builder.GetOrCreateResponseBodyBuilder().AddDirective(std::move(directive));
            }
            if (frameRedirectResponse.ServerDirective) {
                NScenarios::TServerDirective serverDirective;
                *serverDirective.MutablePushTypedSemanticFrameDirective() = std::move(*frameRedirectResponse.ServerDirective);
                builder.GetOrCreateResponseBodyBuilder().AddServerDirective(std::move(serverDirective));
            }
            if (frameRedirectResponse.PlayerFeatures) {
                builder.AddPlayerFeatures(std::move(*frameRedirectResponse.PlayerFeatures));
            }

            return true;
        }
    }

    return false;
}

} // namespace NAlice::NHollywood::NRedirect
