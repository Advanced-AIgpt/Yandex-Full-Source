#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>

#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

const TString VOICEPRINT_DECLINE_INTENT = "alice.voiceprint.decline";

} // namespace

TRunHandleImpl::TRunHandleImpl(TScenarioHandleContext& ctx)
    : VoiceprintCtx_{ctx}
{}

void TRunHandleImpl::LogInfoScenarioState() {
    LOG_INFO(VoiceprintCtx_.Logger) << "Current state: " << VoiceprintCtx_.ScenarioStateProto;
}

bool TRunHandleImpl::CheckVoiceprintSupported() {
    const auto& request = VoiceprintCtx_.Request;
    const auto clientInfo = request.ClientInfo();

    if (!clientInfo.IsSmartSpeaker()) {
        IrrelevantResponse(EIrrelevantType::UnsupportedSurface, clientInfo.Name);
        return false;
    }

    return true;
}

void TRunHandleImpl::IrrelevantResponse(EIrrelevantType type, TStringBuf msg) {
    auto& ctx = VoiceprintCtx_.Ctx;
    auto request = VoiceprintCtx_.Request;
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    Irrelevant(VoiceprintCtx_.Logger, request, builder, type, msg);
    ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

bool TRunHandleImpl::IsValidRegion() {
    return IsValidRegionImpl(VoiceprintCtx_.Ctx, VoiceprintCtx_.Request);
}

void TRunHandleImpl::RenderDeclineResponse() {
    auto& ctx = VoiceprintCtx_.Ctx;
    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), VoiceprintCtx_.Request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
    analyticsInfo.SetIntentName(VOICEPRINT_DECLINE_INTENT);

    VoiceprintCtx_.ScenarioStateProto = TVoiceprintState{};
    bodyBuilder.SetState(VoiceprintCtx_.ScenarioStateProto);

    ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
