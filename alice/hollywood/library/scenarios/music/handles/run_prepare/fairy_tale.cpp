#include "impl.h"

#include <alice/hollywood/library/scenarios/music/fairy_tales/bedtime_tales.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/ondemand.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/playlists.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/semantic_frames.h>

namespace NAlice::NHollywood::NMusic::NImpl {

namespace {

TSemanticFrame CreateFrameWithFairytale(
    const TStringBuf fairytaleId,
    const TStringBuf answerType,
    const bool useThinPlaylist,
    const bool shuffle)
{
    NJson::TJsonValue fairytale;

    fairytale[NAlice::NMusic::SLOT_OBJECT_ID]["name"] = NAlice::NMusic::SLOT_OBJECT_ID;
    fairytale[NAlice::NMusic::SLOT_OBJECT_ID]["type"] = NAlice::NMusic::SLOT_OBJECT_ID;
    fairytale[NAlice::NMusic::SLOT_OBJECT_ID]["value"] = fairytaleId;

    fairytale[NAlice::NMusic::SLOT_OBJECT_TYPE]["name"] = NAlice::NMusic::SLOT_OBJECT_TYPE;
    fairytale[NAlice::NMusic::SLOT_OBJECT_TYPE]["type"] = NAlice::NMusic::SLOT_OBJECT_TYPE;
    fairytale[NAlice::NMusic::SLOT_OBJECT_TYPE]["value"] = answerType;

    if (shuffle) {
        fairytale[NAlice::NMusic::SLOT_ORDER]["name"] = NAlice::NMusic::SLOT_ORDER;
        fairytale[NAlice::NMusic::SLOT_ORDER]["type"] = NAlice::NMusic::SLOT_ORDER_TYPE;
        fairytale[NAlice::NMusic::SLOT_ORDER]["value"] = "shuffle";
    }

    return CreateSpecialAnswerFrame(fairytale, useThinPlaylist).ToProto();
}

const TFrame CreateFairyTalePlaylistFrame(
    TRTLogger& logger,
    const ru::yandex::alice::memento::proto::TUserConfigs& userConfigs,
    const bool useThinPlaylists,
    const bool bedtimeTales,
    const bool shuffle)
{
    TFrame frame{""};
    const NAlice::NMusic::TSpecialPlaylistInfo& playlist = DefaultFairyTalePlaylist(logger, userConfigs, bedtimeTales);
    if (const auto* albumInfo = std::get_if<NAlice::NMusic::TSpecialPlaylistInfo::TAlbum>(&playlist.Info)) {
        frame = TFrame::FromProto(CreateFrameWithFairytale(albumInfo->Id, "album", useThinPlaylists, shuffle));
    } else {
        const auto& playlistInfo = std::get<NAlice::NMusic::TSpecialPlaylistInfo::TPlaylist>(playlist.Info);
        const TString playlistId = TString::Join(playlistInfo.OwnerId, ":", playlistInfo.Kind);
        frame = TFrame::FromProto(CreateFrameWithFairytale(playlistId, "playlist", useThinPlaylists, shuffle));
    }
    LOG_DEBUG(logger) << "Created fairytale playlist frame: " << JsonStringFromProto(frame.ToProto());
    return frame;
}

} // namespace

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::HandleFairyTale() {
    Y_ENSURE(Frame_.Name() == MUSIC_PLAY_FAIRYTALE_FRAME);

    const bool useThinFairyTalePlaylists = Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_PLAYLISTS) &&
        Request_.Interfaces().GetHasAudioClient();
    const auto& userConfigs = Request_.BaseRequestProto().GetMemento().GetUserConfigs();
    const bool isBedtimeTales = IsBedtimeTales(Request_.Input(), Request_.HasExpFlag(NExperiments::EXP_FAIRY_TALES_BEDTIME_TALES));

    // redefine Frame_
    Frame_ = CreateFairyTalePlaylistFrame(
        Logger_,
        userConfigs,
        useThinFairyTalePlaylists,
        isBedtimeTales,
        /* shuffle */ !Request_.HasExpFlag(NExperiments::EXP_FAIRY_TALES_DISABLE_SHUFFLE)
    );

    // thin player can return music play object response
    if (useThinFairyTalePlaylists) {
        return HandleThinClientMusicPlayObject({
            .IsFairyTalePlaylistFrame = true,
            .IsBedtimeTales = isBedtimeTales,
        });
    }

    // otherwise go to BASS
    return Nothing();
}

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::HandleFairyTaleOnDemand() {
    if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_FAIRY_TALES_ENABLE_ONDEMAND)) {
        return Nothing();
    }

    Y_ENSURE(FindFrame(ALICE_FAIRY_TALE_ONDEMAND_FRAME));

    TMaybe<TSemanticFrame> ondemandFrame = TryCreateOnDemandFairyTaleFrame(
            Logger_,
            FindFrame(MUSIC_FAIRY_TALE_FRAME),
            FindFrame(MUSIC_PLAY_FRAME));
    if (ondemandFrame.Defined()) {
        // redefine Frame_
        Frame_ = TFrame::FromProto(ondemandFrame.GetRef());
    }

    // go to BASS
    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
