#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/string/cast.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

void AddRemoveVoiceprintDirectives(TRTLogger& logger, const TVoiceprintRemoveState& removeState, TResponseBodyBuilder& bodyBuilder) {
    // NOTE(klim-roma): despite it is listed in client directives in directives.proto, in MM it is put into server directives
    NScenarios::TDirective serverDirective;
    auto& removeVPDirective = *serverDirective.MutableRemoveVoiceprintDirective();
    removeVPDirective.SetUserId(removeState.GetUid());
    removeVPDirective.SetPersId(removeState.GetPersId());
    LOG_INFO(logger) << "remove_voiceprint directive: " << serverDirective;
    bodyBuilder.AddDirective(std::move(serverDirective));

    if (removeState.GetIsBioCapabilitySupported()) {
        NScenarios::TDirective clientDirective;
        auto& multiaccountRemoveAccountDirective = *clientDirective.MutableMultiaccountRemoveAccountDirective();
        multiaccountRemoveAccountDirective.SetPuid(FromString<uint64_t>(removeState.GetUid()));
        LOG_INFO(logger) << "multiaccount_remove_account directive: " << clientDirective;
        bodyBuilder.AddDirective(std::move(clientDirective));
        bodyBuilder.AddTtsPlayPlaceholderDirective();
    } else {
        LOG_INFO(logger) << "BioCapability is not supported, do not add multiaccount_remove_account directive";
    }
}

} // namespace

bool TApplyPrepareHandleImpl::HandleRemove() {
    if (!ApplyArgs_.HasVoiceprintRemoveState()) {
        return false;
    }
    const auto& removeState = ApplyArgs_.GetVoiceprintRemoveState();

    TApplyResponseBuilder builder(&Nlg_);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    auto renderSlots = NJson::TJsonArray();

    AddRemoveVoiceprintDirectives(Logger_, removeState, bodyBuilder);
    AddSlot(renderSlots, "is_removed", true);

    if (removeState.GetIsBioCapabilitySupported() && Request_.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT)) {
        AddSlot(renderSlots, SLOT_IS_MULTIACCOUNT_ENABLED, true);
    }

    auto renderForm = NJson::TJsonMap();
    renderForm["slots"] = renderSlots;
    TNlgData nlgData{Logger_, Request_};
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    nlgData.Form = renderForm;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_REMOVE_FINISH, "render_result", /* buttons = */ {}, nlgData);

    analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
    analyticsInfo.SetIntentName(TString(REMOVE_FINISH_FRAME));

    auto response = std::move(builder).BuildResponse();
    Ctx_.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    return true;
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
