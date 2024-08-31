#include "impl.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/similar_radio_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/multiroom.h>

#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic::NImpl {

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::HandleThinClientRadio() {
    if (!IsThinRadioSupported(Request_)) {
        return Nothing();
    }

    const auto musicPlayFramePtr = FindFrame(MUSIC_PLAY_FRAME);
    if (!musicPlayFramePtr) {
        return Nothing();
    }
    const auto musicPlayFrame = *musicPlayFramePtr;

    const bool hasSearch = musicPlayFrame.FindSlot(::NAlice::NMusic::SLOT_SEARCH_TEXT);
    if (hasSearch) {
        LOG_INFO(Logger_) << "Not a radio request because it has slot " << ::NAlice::NMusic::SLOT_SEARCH_TEXT;
        return Nothing();
    }
    const bool hasGenerative = musicPlayFrame.FindSlot(::NAlice::NMusic::SLOT_GENERATIVE_STATION);
    if (hasGenerative) {
        LOG_INFO(Logger_) << "Not a radio request because it has slot " << ::NAlice::NMusic::SLOT_GENERATIVE_STATION;
        return Nothing();
    }
    const bool hasFmRadio = Request_.Interfaces().GetHasAudioClient() &&
        Request_.Input().FindSemanticFrame(FM_RADIO_PLAY_FRAME) &&
        Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FM_RADIO);
    if (hasFmRadio) {
        LOG_INFO(Logger_) << "Not a radio request because it is FmRadio request";
        return Nothing();
    }

    const bool hasForbiddenMultiroom = HasForbiddenMultiroom(Logger_, Request_, musicPlayFrame);
    if (hasForbiddenMultiroom) {
        LOG_INFO(Logger_) << "Not a radio request because it hasForbiddenMultiroom";
        return Nothing();
    }

    TVector<TString> radioStationIds = GetAllRadioStationIds(musicPlayFrame, Logger_);
    LOG_INFO(Logger_) << "GetAllRadioStationIds result is ["
                     << JoinRange(", ", radioStationIds.begin(), radioStationIds.end())
                     << "]";

    if (radioStationIds.empty()) {
        const auto slotMissingType = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_MISSING_TYPE);
        const auto slotDecline = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_DECLINE);
        const auto slotSpecialAnswerInfo = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO);
        const auto slotNovelty = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_NOVELTY);
        const auto slotPersonality = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_PERSONALITY);
        const auto slotPlaylist = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_PLAYLIST);
        const auto slotSpecialPlaylist = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_SPECIAL_PLAYLIST);
        const auto slotObjectId = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_OBJECT_ID);
        const auto slotAlarmId = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_ALARM_ID);
        const auto slotNeedSimilar = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_NEED_SIMILAR);
        LOG_INFO(Logger_) << "slotMissingType = " << static_cast<bool>(slotMissingType)
                         << ", slotDecline = " << static_cast<bool>(slotDecline)
                         << ", slotSpecialAnswerInfo = " << static_cast<bool>(slotSpecialAnswerInfo)
                         << ", slotNovelty = " << static_cast<bool>(slotNovelty)
                         << ", slotPersonality = " << static_cast<bool>(slotPersonality)
                         << ", slotPlaylist = " << static_cast<bool>(slotPlaylist)
                         << ", slotSpecialPlaylist = " << static_cast<bool>(slotSpecialPlaylist)
                         << ", slotObjectId = " << static_cast<bool>(slotObjectId)
                         << ", slotAlarmId = " << static_cast<bool>(slotAlarmId)
                         << ", slotNeedSimilar = " << static_cast<bool>(slotNeedSimilar);

        if (!slotMissingType &&
            !slotDecline &&
            !slotSpecialAnswerInfo &&
            !slotNovelty &&
            !slotPersonality &&
            !slotPlaylist &&
            !slotSpecialPlaylist &&
            !slotObjectId &&
            !slotAlarmId)
        {
            if (slotNeedSimilar && MusicQueue_.QueueSize() > 0) {
                if (!MusicQueue_.IsGenerative() && !MusicQueue_.IsRadio()) {
                    radioStationIds = {SimilarRadioId(MusicQueue_.ContentId())};
                } else if (MusicQueue_.IsRadio() && MusicQueue_.HasCurrentItem()) {
                    radioStationIds = {TString::Join("track:", MusicQueue_.CurrentItem().GetTrackId())};
                } else {
                    LOG_INFO(Logger_) << "Requested need_similar radio, but there is no scenario state, falling back to "
                                     << USER_RADIO_STATION_ID;
                    radioStationIds = {USER_RADIO_STATION_ID};
                }
            } else {
                LOG_INFO(Logger_) << "There is action_request=autoplay slot but no other slots, "
                                 << "that is why this is radio " << USER_RADIO_STATION_ID;
                radioStationIds = {USER_RADIO_STATION_ID};
            }
        } else {
            LOG_INFO(Logger_) << "Not a radio request because it has other non-radio slots";
            return Nothing();
        }
    }

    Y_ENSURE(!radioStationIds.empty());
    LOG_INFO(Logger_) << "Handling thin client radio, detected stationIds are: " << JoinSeq(", ", radioStationIds);

    auto args = MakeMusicArguments(Logger_, Request_, TMusicArguments_EExecutionFlowType_ThinClientDefault,
                                         /* isNewContentRequestedByUser= */ true);

    auto& radioRequest = *args.MutableRadioRequest();

    const auto filtrationMode = Request_.BaseRequestProto().GetUserPreferences().GetFiltrationMode();
    const bool isUserOnYourWaveSelected = radioStationIds.size() == 1 &&
                                          radioStationIds.at(0) == USER_RADIO_STATION_ID;

    if (isUserOnYourWaveSelected && Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FORCE_ICHWILL_ONYOURWAVE)) {
        args.MutablePlaybackOptions()->SetUseIchwill(true);
    }

    if (!isUserOnYourWaveSelected && !radioStationIds.empty() &&
        filtrationMode == NScenarios::TUserPreferences_EFiltrationMode_Safe) {
        if (Find(radioStationIds, FORCHILDREN_STATION_ID) == radioStationIds.end()) {
            radioRequest.SetNeedRestrictedContentSettingsAttention(true);
        }
        radioStationIds.clear();
        radioStationIds.push_back(TString(FORCHILDREN_STATION_ID));
        LOG_INFO(Logger_) << "Falling back to " << FORCHILDREN_STATION_ID
                         << " radio stationId because FiltrationMode is Safe";
    }

    for (const auto& id : radioStationIds) {
        radioRequest.AddStationIds(id);
    }

    const TStringBuf uid = GetUid(Request_);
    args.MutableAccountStatus()->SetUid(uid.data(), uid.size());

    if (const auto slot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_STREAM)) {
        TStringBuf slotValue = slot->Value.AsString();
        if (slotValue == USER_RADIO_STATION_ID) {
            args.MutablePlaybackOptions()->SetUseIchwill(
                !Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_DISABLE_ICHWILL_MYWAVE));
        }
    }

    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
    response.SetContinueArguments(args);
    response.SetFeaturesIntent(musicPlayFrame.Name());

    return *std::move(response).BuildResponse();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
