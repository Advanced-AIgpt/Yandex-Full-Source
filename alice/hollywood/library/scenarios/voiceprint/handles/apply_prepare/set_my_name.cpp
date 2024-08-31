#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/proto/proto.h>

#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

bool IsGuestSetMyNameRequest(const TVoiceprintSetMyNameState& setMyNameState) {
    return !setMyNameState.GetGuestUid().Empty() && setMyNameState.GetGuestUid() != setMyNameState.GetOwnerUid();
}

} // namespace

bool TApplyPrepareHandleImpl::HandleSetMyName() {
    if (!ApplyArgs_.HasVoiceprintSetMyNameState()) {
        return false;
    }

    TApplyResponseBuilder builder(&Nlg_);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    const auto clientInfo = Request_.ClientInfo();
    auto renderSlots = NJson::TJsonArray();
    const auto& setMyNameState = ApplyArgs_.GetVoiceprintSetMyNameState();
    const auto& userName = setMyNameState.GetUserName();

    AddSlot(renderSlots, SLOT_USER_NAME, userName);
    if (!setMyNameState.GetOldUserName().Empty()) {
        AddSlot(renderSlots, SLOT_OLD_USER_NAME, setMyNameState.GetOldUserName());
    }

    if (IsGuestSetMyNameRequest(setMyNameState)) {
        if (Request_.HasExpFlag(EXP_HW_VOICEPRINT_UPDATE_GUEST_DATASYNC)) {
            const auto dsGuestUserNameKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::UserName, setMyNameState.GetPersId());
            auto setUserNameDirective = MakeUpdateDatasyncDirective(Logger_, dsGuestUserNameKey, userName, NScenarios::TServerDirective::TMeta::CurrentUser);
            bodyBuilder.AddServerDirective(std::move(setUserNameDirective));
        }
    } else {
        const auto dsUserNameKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserSpecificKey::UserName);
        auto setUserNameDirective = MakeUpdateDatasyncDirective(Logger_, dsUserNameKey, userName, NScenarios::TServerDirective::TMeta::DeviceOwner);
        bodyBuilder.AddServerDirective(std::move(setUserNameDirective));
    }

    auto renderForm = NJson::TJsonMap();
    renderForm["slots"] = renderSlots;

    TNlgData nlgData{Logger_, Request_};
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    nlgData.Form = renderForm;

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_SET_MY_NAME, "render_result", /* buttons = */ {}, nlgData);

    analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
    analyticsInfo.SetIntentName(TString(SET_MY_NAME_FRAME));

    auto response = std::move(builder).BuildResponse();
    Ctx_.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    return true;
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
