#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/protos/data/scenario/voiceprint/personalization_data.pb.h>

#include <alice/library/proto/proto.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

TString MakeReadyPhraseFrom(TRTLogger& logger, NAlice::NData::NVoiceprint::EGender gender) {
    switch (gender) {
    case NAlice::NData::NVoiceprint::Male:
        return "я готов";
    case NAlice::NData::NVoiceprint::Female:
        return "я готова";
    default:
        LOG_WARN(logger) << "Can't make ready phrase for bass because of unexpected gender value: "
                         << NAlice::NData::NVoiceprint::EGender_Name(gender);
        return "";
    }
}

} // namespace

bool TApplyPrepareHandleImpl::HandleEnroll() {
    if (!ApplyArgs_.HasVoiceprintEnrollState()) {
        return false;
    }

    TApplyResponseBuilder builder(&Nlg_);
    const auto clientInfo = Request_.ClientInfo();

    const auto dsIncognitoUidKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserSpecificKey::GuestUID);
    const auto incognitoUidPtr = Request_.GetPersonalDataString(dsIncognitoUidKey);

    auto& enrollState = *ApplyArgs_.MutableVoiceprintEnrollState();
    UpdateVoiceprintEnrollState(enrollState);
    const auto guestPuid = enrollState.GetGuestPuid();

    if (guestPuid || (incognitoUidPtr && *incognitoUidPtr && !Request_.HasExpFlag(EXP_HW_VOICEPRINT_ENROLLMENT_FINISH_OVER_BASS))) {
        auto& bodyBuilder = builder.CreateResponseBodyBuilder();
        auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
        const auto userName = enrollState.GetUserName();

        const auto gender = to_lower(NAlice::NData::NVoiceprint::EGender_Name(enrollState.GetGenderMementoReady()));

        auto expectsGuestEnrollmentFinishFrame = guestPuid &&
                                                       enrollState.GetCurrentStage() == TVoiceprintEnrollState::Complete &&
                                                       Request_.HasExpFlag(EXP_HW_VOICEPRINT_UPDATE_GUEST_DATASYNC);

        if (guestPuid) {
            if (expectsGuestEnrollmentFinishFrame) {
                TVoiceprintState scState;
                scState.MutableVoiceprintEnrollState()->CopyFrom(enrollState);
                bodyBuilder.SetState(scState);
            } else if (Request_.HasExpFlag(EXP_HW_VOICEPRINT_UPDATE_GUEST_DATASYNC)) {
                const auto& persId = enrollState.GetPersId();

                const auto dsGuestUserNameKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::UserName, persId);
                auto setUserNameDirective = MakeUpdateDatasyncDirective(Logger_, dsGuestUserNameKey, userName, NScenarios::TServerDirective::TMeta::CurrentUser);
                bodyBuilder.AddServerDirective(std::move(setUserNameDirective));

                const auto dsGuestGenderKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::Gender, persId);
                auto setGenderDirective = MakeUpdateDatasyncDirective(Logger_, dsGuestGenderKey, gender, NScenarios::TServerDirective::TMeta::CurrentUser);
                bodyBuilder.AddServerDirective(std::move(setGenderDirective));
            }
        } else {
            const auto dsUserNameKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserSpecificKey::UserName);
            auto setUserNameDirective = MakeUpdateDatasyncDirective(Logger_, dsUserNameKey, userName, NScenarios::TServerDirective::TMeta::DeviceOwner);
            bodyBuilder.AddServerDirective(std::move(setUserNameDirective));

            const auto dsGenderKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EUserSpecificKey::Gender);
            auto setGenderDirective = MakeUpdateDatasyncDirective(Logger_, dsGenderKey, gender, NScenarios::TServerDirective::TMeta::DeviceOwner);
            bodyBuilder.AddServerDirective(std::move(setGenderDirective));

            AddSaveVoiceprintDirective(Logger_, enrollState, bodyBuilder);
        }

        if (Request_.HasExpFlag(EXP_HW_VOICEPRINT_ENROLLMENT_DIRECTIVES) && enrollState.GetCurrentStage() != TVoiceprintEnrollState::Finish) {
            AddEnrollmentFinishDirective(Logger_, enrollState, bodyBuilder, expectsGuestEnrollmentFinishFrame);
        }

        if (!expectsGuestEnrollmentFinishFrame) {
            auto renderSlots = NJson::TJsonArray();
            AddSlot(renderSlots, "created_uid", true);
            AddSlot(renderSlots, "user_name", userName);

            if (enrollState.GetIsBioCapabilitySupported() && Request_.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT)) {
                AddSlot(renderSlots, SLOT_IS_MULTIACCOUNT_ENABLED, true);
            } else {
                bodyBuilder.SetShouldListen(true);
            }

            auto renderForm = NJson::TJsonMap();
            renderForm["slots"] = renderSlots;

            TNlgData nlgData{Logger_, Request_};
            nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
            nlgData.Form = renderForm;

            bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_ENROLL_FINISH, "render_result", /* buttons = */ {}, nlgData);
        }

        analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
        analyticsInfo.SetIntentName(TString(ENROLL_FINISH_FRAME));

        auto response = std::move(builder).BuildResponse();
        Ctx_.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return true;
    }

    // TODO: it's better not emulate finish collect frame and just call BASS for register kolonkish instead all this stuff
    auto readyPhrase = MakeReadyPhraseFrom(Logger_, enrollState.GetGenderMementoReady());
    TFrame frameEssence(std::move(TString(ENROLL_COLLECT_FRAME)));
    frameEssence.AddSlot(TSlot{"user_name", "string", TSlot::TValue{enrollState.GetUserName()}});
    frameEssence.AddSlot(TSlot{"user_name_frozen", "string", TSlot::TValue{enrollState.GetUserName()}});
    frameEssence.AddSlot(TSlot{"ready", "string", TSlot::TValue{readyPhrase}});
    frameEssence.AddSlot(TSlot{"ready_frozen", "string", TSlot::TValue{readyPhrase}});
    frameEssence.AddSlot(TSlot{"phrases_count", "int", TSlot::TValue{ToString(MAX_PHARSES)}});
    const auto reqRange = enrollState.GetRequestIds();
    const auto requests = TString::Join("[\"", JoinRange("\",\"", reqRange.begin(), reqRange.end()), "\"]");
    frameEssence.AddSlot(TSlot{"voice_requests", "list", TSlot::TValue{requests}});

    auto& inputProto = *RequestProto_.mutable_input();
    inputProto.ClearText();
    auto& voice = *inputProto.MutableVoice();
    auto& biometry = *voice.mutable_biometryscoring();
    biometry.SetStatus("ok");

    TBlackBoxUserInfo userInfo;
    userInfo.SetUid(enrollState.GetUid());
    NScenarios::TDataSource dsUserInfo;
    *dsUserInfo.MutableUserInfo() = userInfo;
    const THashMap<EDataSourceType, const NScenarios::TDataSource*> usedDataSource = {{BLACK_BOX, &dsUserInfo}};
    const bool forbidWebSearch = true;
    auto requestBass = PrepareBassVinsRequest(
        Logger_,
        Request_,
        Request_.Input(),
        frameEssence,
        /* sourceTextProvider= */ nullptr,
        Ctx_.RequestMeta,
        /* imageSearch= */ false,
        Ctx_.AppHostParams,
        forbidWebSearch,
        usedDataSource
    );
    AddBassRequestItems(Ctx_, requestBass);
    return true;
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
