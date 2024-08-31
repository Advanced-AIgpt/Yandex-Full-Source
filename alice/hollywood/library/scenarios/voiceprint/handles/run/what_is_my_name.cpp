#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/server_biometry.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/library/biometry/biometry.h>
#include <alice/hollywood/library/frame/frame.h>

#include <alice/library/data_sync/data_sync.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

NScenarios::TScenarioRunResponse TRunHandleImpl::RenderWhatIsMyNameResponse(
    const TFrame& frame,
    const NJson::TJsonArray& renderSlots,
    TNlgData& nlgData,
    bool shouldListen
)
{
    const auto& request = VoiceprintCtx_.Request;
    const auto& ctx = VoiceprintCtx_.Ctx;
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    auto renderForm = NJson::TJsonMap();
    renderForm["slots"] = renderSlots;
    nlgData.Form = renderForm;

    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
    analyticsInfo.SetIntentName(frame.Name());

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_WHAT_IS_MY_NAME, "render_result", /* buttons = */ {}, nlgData);
    bodyBuilder.SetShouldListen(shouldListen);

    return *std::move(builder).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunHandleImpl::RenderUnknownUser(
    const TFrame& frame,
    NJson::TJsonArray& renderSlots,
    TNlgData& nlgData,
    bool shouldListen,
    TStringBuf msg
)
{
    LOG_INFO(VoiceprintCtx_.Logger) << "User is not recognized: " << msg;
    AddSlot(renderSlots, SLOT_IS_KNOWN, false);
    return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, shouldListen);
}

TMaybe<NScenarios::TScenarioRunResponse> TRunHandleImpl::HandleWhatIsMyName() {
    auto frame = VoiceprintCtx_.FindFrame(WHAT_IS_MY_NAME_FRAME);
    if (!frame) {
        return Nothing();
    }

    auto& logger = VoiceprintCtx_.Logger;

    NJson::TJsonArray renderSlots;
    TNlgData nlgData{logger, VoiceprintCtx_.Request};
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);

    if (IsBioCapabilitySupported(VoiceprintCtx_.Request)) {
        return HandleWhatIsMyNameUsingClientBiometry(*frame, renderSlots, nlgData);
    } else {
        return HandleWhatIsMyNameUsingServerBiometry(*frame, renderSlots, nlgData);
    }
}

NScenarios::TScenarioRunResponse TRunHandleImpl::HandleWhatIsMyNameUsingClientBiometry(
    const TFrame& frame,
    NJson::TJsonArray& renderSlots,
    TNlgData& nlgData
)
{
    auto& logger = VoiceprintCtx_.Logger;
    const auto& request = VoiceprintCtx_.Request;
    const auto* userInfo = VoiceprintCtx_.UserInfo;

    if (!userInfo) {
        LOG_ERROR(logger) << "Failed to get uid from BlackBox user info";
        nlgData.Context["attentions"][ATTENTION_SERVER_ERROR] = true;
        return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
    }

    const auto* guestData = GetGuestDataProto(request);
    const auto* guestOptions = GetGuestOptionsProto(request);
    if (!HasMatch(guestOptions)) {
        return RenderUnknownUser(frame, renderSlots, nlgData, /* shouldListen = */ true, "no match");
    }

    if (!guestOptions->HasYandexUID() || guestOptions->GetYandexUID().Empty()) {
        LOG_ERROR(logger) << "GuestOptions data source is present, but has empty YandexUID";
        nlgData.Context["attentions"][ATTENTION_SERVER_ERROR] = true;
        return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
    }

    if (!guestData) {
        LOG_ERROR(logger) << "Has match, but GuestData data source is not present";
        nlgData.AddAttention(ATTENTION_SERVER_ERROR);
        return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
    }

    auto userName = GetUserNameFromDataSync(request, userInfo->GetUid(), guestOptions->GetYandexUID(), guestData, guestOptions->GetPersId());
    if (!userName) {
        LOG_ERROR(logger) << "Failed to get user's name from DataSync";
        nlgData.Context["attentions"][ATTENTION_SERVER_ERROR] = true;
        return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
    }

    nlgData.Context["attentions"][ATTENTION_SILENT_ENROLL_MODE] = true;
    AddSlot(renderSlots, SLOT_IS_KNOWN, true);
    AddSlot(renderSlots, SLOT_USER_NAME, *userName);

    return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
}

NScenarios::TScenarioRunResponse TRunHandleImpl::HandleWhatIsMyNameUsingServerBiometry(
    const TFrame& frame,
    NJson::TJsonArray& renderSlots,
    TNlgData& nlgData
)
{
    auto& logger = VoiceprintCtx_.Logger;
    const auto& request = VoiceprintCtx_.Request;
    const auto* userInfo = VoiceprintCtx_.UserInfo;

    auto isValidRegion = IsValidRegion();
    if (!isValidRegion) {
        LOG_INFO(logger) << "Invalid region is detected. Enrollment won't be suggested";
        nlgData.Context["attentions"][ATTENTION_INVALID_REGION] = true;
    }

    if (!userInfo) {
        LOG_ERROR(logger) << "Failed to get uid from BlackBox user info";
        nlgData.Context["attentions"][ATTENTION_SERVER_ERROR] = true;
        return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
    }
    auto uid = userInfo->GetUid();

    auto biometry = ProcessServerBiometry(logger, request, uid,
                                                NBiometry::TBiometry::EMode::HighTPR,
                                                /* addMaxAccuracyIncognitoCheck = */ false);
    if (!biometry) {
        return RenderUnknownUser(frame, renderSlots, nlgData, /* shouldListen = */ isValidRegion,
                          "no server biometry found in request");
    }

    if (biometry->IsIncognitoUser) {
        AddSlot(renderSlots, SLOT_IS_TOO_MANY_ENROLLED_USERS, true);
        return RenderUnknownUser(frame, renderSlots, nlgData, /* shouldListen = */ false,
                          "incognito user is detected");
    }

    nlgData.Context["attentions"][ATTENTION_SILENT_ENROLL_MODE] = true;
    AddSlot(renderSlots, SLOT_IS_KNOWN, true);
    AddSlot(renderSlots, SLOT_USER_NAME, biometry->UserName);

    return RenderWhatIsMyNameResponse(frame, renderSlots, nlgData, /* shouldListen = */ false);
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
