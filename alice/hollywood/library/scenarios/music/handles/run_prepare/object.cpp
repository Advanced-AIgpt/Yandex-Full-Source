#include "impl.h"

#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic::NImpl {

namespace {

const TVector<std::pair<TMusicPlayObjectTypeSlot_EValue, TStringBuf>> MUSIC_PLAY_OBJECT_TYPES_EXPERIMENTS = {
    {TMusicPlayObjectTypeSlot_EValue_Radio, NExperiments::EXP_HW_MUSIC_THIN_CLIENT},
    {TMusicPlayObjectTypeSlot_EValue_Playlist, EXP_HW_MUSIC_THIN_CLIENT_PLAYLIST},
    {TMusicPlayObjectTypeSlot_EValue_Generative, NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE},
};

}

NScenarios::TScenarioRunResponse
TRunPrepareHandleImpl::HandleThinClientMusicPlayObject(TMusicPlayObjectParams params) {
    NJson::TJsonValue musicArguments;
    musicArguments["execution_flow_type"] =
        TMusicArguments_EExecutionFlowType_Name(TMusicArguments_EExecutionFlowType_ThinClientDefault);
    musicArguments["puid"] = GetUid(Request_);

    auto opts = GetCommonPlaybackOptions(Frame_);

    auto& musicSearchResult = musicArguments["music_search_result"];

    if (params.IsFairyTalePlaylistFrame) {
        auto& fairyTaleArguments = musicArguments["fairy_tale_arguments"];
        fairyTaleArguments["is_fairy_tale_subscenario"] = true;
        if (params.IsBedtimeTales) {
            fairyTaleArguments["is_bedtime_tales"] = true;
        }
    }

    if (const auto slot = Frame_.FindSlot(NAlice::NMusic::SLOT_OBJECT_ID)) {
        musicSearchResult["content_id"] = slot->Value.AsString();
    }
    if (const auto slot = Frame_.FindSlot(NAlice::NMusic::SLOT_OBJECT_TYPE)) {
        auto objectType = slot->Value.AsString();
        objectType.to_lower();
        musicSearchResult["content_type"] = objectType;
    }

    if (const auto slot = Frame_.FindSlot(NAlice::NMusic::SLOT_OFFSET_SEC)) {
        if (auto maybeValue = slot->Value.As<double>()) {
            musicArguments["offset_ms"] = *maybeValue * 1000;
        }
    }

    if (const auto slot = Frame_.FindSlot(NAlice::NMusic::SLOT_START_FROM_TRACK_ID)) {
        opts["start_from_track_id"] = slot->Value.AsString();
    }

    if (const auto slot = Frame_.FindSlot(NAlice::NMusic::SLOT_ORDER)) {
        if (slot->Value.AsString() == "shuffle") {
            opts["shuffle"] = true;
        }
    }

    if (const auto slot = Frame_.FindSlot(NAlice::NMusic::SLOT_OFFSET)) {
        opts["offset"] = slot->Value.AsString();
    }

    if (const auto repeatSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_REPEAT)) {
        if (repeatSlot->Value.AsString() == "One") {
            opts["repeat_type"] = RepeatTrack;
        } else if (repeatSlot->Value.AsString() == "All") {
            opts["repeat_type"] = RepeatAll;
        }
    }

    if (opts["from"] == "pult" && musicSearchResult["content_id"] == USER_RADIO_STATION_ID) {
        opts["use_ichwill"] = !Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_DISABLE_ICHWILL_MYWAVE);
    }

    musicArguments["playback_options"] = std::move(opts);
    Y_ENSURE(UserInfo_);
    musicArguments["account_status"]["uid"] = UserInfo_->GetUid();
    musicArguments["account_status"]["has_plus"] = UserInfo_->GetHasYandexPlus();
    musicArguments["account_status"]["has_music_subscription"] = UserInfo_->GetHasMusicSubscription();
    musicArguments["account_status"]["music_subscription_region_id"] = UserInfo_->GetMusicSubscriptionRegionId();

    musicArguments["ambient_sound_arguments"]["is_ambient_sound_request"] = HasMusicPlayAmbientSoundRequest_;

    musicArguments["is_new_content_requested_by_user"] = true;

    const auto* environmentStateProto = GetEnvironmentStateProto(Request_);
    if (environmentStateProto) {
        FillEnvironmentState(musicArguments, *environmentStateProto);
    }

    const auto* guestOptions = GetGuestOptionsProto(Request_);
    if (IsClientBiometryModeRunRequest(Logger_, Request_, guestOptions)) {
        FillGuestCredentials(musicArguments, *guestOptions);
    }

    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};

    LOG_INFO(Logger_) << "Handling thin client music play object with arguments " << musicArguments << Endl;

    // TODO(zhigan): add histogram scope for PackProto
    auto applyArgs = JsonToProto<TMusicArguments>(musicArguments, /* validateUtf8= */ true,
                                                    /* ignoreUnknownFields= */ true);
    response.SetContinueArguments(std::move(applyArgs));

    return *std::move(response).BuildResponse();
}

NScenarios::TScenarioRunResponse TRunPrepareHandleImpl::HandleThinClientMusicPlayObject() {
    return HandleThinClientMusicPlayObject(TMusicPlayObjectParams{});
}

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::TryHandleThinClientMusicPlayObject() {
    if (!Request_.Interfaces().GetHasAudioClient()) {
        return Nothing();
    }

    const TPtrWrapper<TSlot> objectTypeSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_OBJECT_TYPE);
    Y_ENSURE(objectTypeSlot);

    if (objectTypeSlot->Value.AsString() == TMusicPlayObjectTypeSlot_EValue_Name(TMusicPlayObjectTypeSlot_EValue_Generative) &&
        Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE))
    {
        return HandleThinClientMusicPlayObject();
    }

    if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT)) {
        return Nothing();
    }

    for (const auto& [type, exp] : MUSIC_PLAY_OBJECT_TYPES_EXPERIMENTS) {
        if (objectTypeSlot->Value.AsString() == TMusicPlayObjectTypeSlot_EValue_Name(type) &&
            !Request_.HasExpFlag(exp))
        {
            return Nothing();
        }
    }

    // Additional limiter for radio
    if (objectTypeSlot->Value.AsString() == TMusicPlayObjectTypeSlot_EValue_Name(TMusicPlayObjectTypeSlot_EValue_Radio) &&
        !IsThinRadioSupported(Request_))
    {
        return Nothing();
    }

    if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_ALARM)) {
        for (const auto& formSlot : Frame_.Slots()) {
            if (formSlot.Name == NAlice::NMusic::SLOT_ALARM_ID) {
                return Nothing();
            }
        }
    }

    return HandleThinClientMusicPlayObject();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
