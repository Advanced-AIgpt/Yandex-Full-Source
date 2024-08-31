#include "show_view_builder.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/track_parser.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/objects/music.pb.h>
#include <alice/protos/data/scenario/music/content_id.pb.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>
#include <alice/protos/data/scenario/music/player.pb.h>

#include <util/generic/vector.h>

namespace NAlice::NHollywood::NMusic {

namespace {

template<typename TDivArtists, typename TArtists>
void FillArtistsInfo(TDivArtists& divArtists, const TArtists& artists) {
    for (const auto& artist : artists) {
        auto& divArtist = *divArtists.Add();
        divArtist.SetId(artist.GetId());
        divArtist.SetName(artist.GetName());
        divArtist.SetComposer(artist.GetComposer());
        divArtist.SetIsVarious(artist.GetVarious());
    }
}

void FillAlbumInfo(NData::TAlbum& album, const TQueueItem::TTrackInfo& trackInfo) {
    album.SetId(trackInfo.GetAlbumId());
    album.SetTitle(trackInfo.GetAlbumTitle());
    album.SetGenre(trackInfo.GetGenre());
    album.SetCoverUri(trackInfo.GetAlbumCoverUrl());
    FillArtistsInfo(*album.MutableArtists(), trackInfo.GetAlbumArtists());
}

void FillTracksInfo(NData::TMusicPlayerData& musicPlayerData,
                    TDeque<const NJson::TJsonValue*> tracks,
                    const THashSet<TStringBuf>& likedTrackIds,
                    const THashSet<TStringBuf>& dislikedTrackIds)
{
    for (const auto* trackJson : tracks) {
        const auto item = ParseTrack(*trackJson);

        auto& queueItem = *musicPlayerData.AddQueueItems();
        queueItem.SetId(item.GetTrackId());
        queueItem.SetTitle(item.GetTitle());
        queueItem.SetArtImageUrl(item.GetCoverUrl());
        queueItem.SetDurationMs(item.GetDurationMs());

        auto& trackInfo = *queueItem.MutableTrackInfo();
        trackInfo.SetAlbumId(item.GetTrackInfo().GetAlbumId());

        if (likedTrackIds.contains(queueItem.GetId())) {
            trackInfo.SetLikeStatus(NData::TQueueItem_TTrackInfo_ELikeStatus_Liked);
        } else if (dislikedTrackIds.contains(queueItem.GetId())) {
            trackInfo.SetLikeStatus(NData::TQueueItem_TTrackInfo_ELikeStatus_Disliked);
        } else {
            trackInfo.SetLikeStatus(NData::TQueueItem_TTrackInfo_ELikeStatus_None);
        }

        FillArtistsInfo(*queueItem.MutableArtists(), item.GetTrackInfo().GetArtists());
    }
}

void FillColors(NData::TMusicPlayerData& musicPlayerData, const TAvatarColorsRequestHelper<ERequestPhase::After>& avatarColors) {
    if (avatarColors.HasResponse() && avatarColors.GetMainColor()) {
        auto& colors = *musicPlayerData.MutableColors();
        colors.SetMainColor(avatarColors.GetMainColor());
        colors.SetSecondColor(avatarColors.GetSecondColor());
    }
}

void FillFmRadioQueueItems(NData::TMusicPlayerData& musicPlayerData, const TVector<TQueueItem>& items) {
    for (const auto& item : items) {
        auto& queueItem = *musicPlayerData.AddQueueItems();
        queueItem.SetId(item.GetTrackId());
        queueItem.SetTitle(item.GetTitle());
        queueItem.SetArtImageUrl(item.GetCoverUrl());
        queueItem.MutableFmRadioInfo()->SetFrequency(item.GetFmRadioInfo().GetFrequency());
        queueItem.MutableFmRadioInfo()->SetColor(item.GetFmRadioInfo().GetColor());
    }
}

} // namespace

void FillContentId(NData::NMusic::TContentId& contentId, const TContentId& musicQueueContentId) {
    switch (musicQueueContentId.GetType()) {
        case TContentId_EContentType_Track:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_Track);
            break;
        case TContentId_EContentType_Album:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_Album);
            break;
        case TContentId_EContentType_Artist:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_Artist);
            break;
        case TContentId_EContentType_Playlist:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_Playlist);
            break;
        case TContentId_EContentType_Radio:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_Radio);
            break;
        case TContentId_EContentType_Generative:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_Generative);
            break;
        case TContentId_EContentType_FmRadio:
            contentId.SetType(::NAlice::NData::NMusic::TContentId_EContentType_FmRadio);
            break;
        case TContentId_EContentType_TContentId_EContentType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TContentId_EContentType_TContentId_EContentType_INT_MAX_SENTINEL_DO_NOT_USE_:
            Y_UNREACHABLE();
    }

    contentId.SetId(musicQueueContentId.GetId());
    for (const auto& id : musicQueueContentId.GetIds()) {
        contentId.AddIds(id);
    }
}

TShowViewBuilder::TShowViewBuilder(TRTLogger& logger,
                                   const TMusicQueueWrapper& mq,
                                   const TShowViewBuilderSources sources,
                                   const TScenarioApplyRequestWrapper* request)
    : Logger_{&logger}
    , MusicQueue_{mq}
    , Sources_{sources}
    , Request_{request}
{
    Y_ENSURE(MusicQueue_.HasCurrentItem());
}

NRenderer::TDivRenderData TShowViewBuilder::BuildRenderData() const {
    THashSet<TStringBuf> likedTrackIds;
    THashSet<TStringBuf> dislikedTrackIds;
    if (Sources_.LikesTracks) {
        likedTrackIds = Sources_.LikesTracks->GetTrackIds();
    }
    if (Sources_.DislikesTracks) {
        dislikedTrackIds = Sources_.DislikesTracks->GetTrackIds();
    }
    LOG_INFO(*Logger_) << "Liked tracks count: " << likedTrackIds.size() << ", disliked tracks count: " << dislikedTrackIds.size();

    NRenderer::TDivRenderData divRenderData;
    divRenderData.SetCardId("music.scenario.player.div.card");

    auto& musicScenarioData = *divRenderData.MutableScenarioData()->MutableMusicPlayerData();

    auto& track = *musicScenarioData.MutableTrack();
    const auto& currentItem = MusicQueue_.CurrentItem();
    track.SetId(currentItem.GetTrackId());
    track.SetTitle(currentItem.GetTitle());
    track.SetArtImageUrl(currentItem.GetCoverUrl());
    track.SetIsLiked(likedTrackIds.contains(track.GetId()));
    track.SetIsDisliked(dislikedTrackIds.contains(track.GetId()));
    track.SetDurationMs(currentItem.GetDurationMs());
    *musicScenarioData.MutableContentInfo() = MusicQueue_.ContentInfo();
    FillContentId(*musicScenarioData.MutableContentId(), MusicQueue_.ContentId());
    if (Sources_.AvatarColors) {
        FillColors(musicScenarioData, *Sources_.AvatarColors);
    }
    if (const auto shuffled = MusicQueue_.GetMetadataShuffled(); shuffled.Defined()) {
        musicScenarioData.SetShuffleModeOn(*shuffled);
    }
    if (const auto repeatType = MusicQueue_.GetMetadataRepeatType(); repeatType.Defined()) {
        musicScenarioData.SetRepeatMode(static_cast<NData::ERepeatMode>(*repeatType));
    }

    if (currentItem.HasTrackInfo()) {
        FillAlbumInfo(*track.MutableAlbum(), currentItem.GetTrackInfo());
        FillArtistsInfo(*track.MutableArtists(), currentItem.GetTrackInfo().GetArtists());
        if (Sources_.NeighboringTracks && Request_) {
            const auto [leftTrackPosition, rightTrackPosition] = MusicQueue_.GetNeighboringTracksBound(*Request_);
            FillTracksInfo(musicScenarioData,
                           Sources_.NeighboringTracks->GetTracks(leftTrackPosition, rightTrackPosition),
                           likedTrackIds,
                           dislikedTrackIds);
        }
    } else if (currentItem.HasFmRadioInfo()) {
        if (Sources_.FmRadioList) {
            FillFmRadioQueueItems(musicScenarioData, *Sources_.FmRadioList);
        }
    }

    LOG_INFO(*Logger_) << "Div render data: " << divRenderData;

    return divRenderData;
}

} // NAlice::NHollywood::NMusic
