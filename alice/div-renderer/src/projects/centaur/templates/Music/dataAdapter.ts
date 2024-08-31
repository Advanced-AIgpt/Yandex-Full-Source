import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { IMusicArtist, IMusicTrackInfo } from './types';
import { ExpFlags } from '../../expFlags';
import { IRequestState } from '../../../../common/types/common';

type ITArtist = NAlice.NData.ITArtist;
type ITQueueItem = NAlice.NData.ITQueueItem;

function artistDataAdapter(input: ITArtist): IMusicArtist {
    return {
        id: input.Id,
        isComposer: input.Composer,
        name: input.Name,
        isVarious: input.IsVarious,
    };
}

export function musicTrackDataAdapter(
    input: ITQueueItem | null | undefined,
): IMusicTrackInfo | null {
    if (!input) {
        return null;
    }

    if (!input.Id || !input.Title) {
        return null;
    }

    const artists = input.Artists && compact(input.Artists.map(artistDataAdapter));

    return {
        id: input.Id,
        altImageUrl: input.ArtImageUrl,
        title: input.Title,
        artists,
        subtype: input.Subtype,
        imageBackground: input.FmRadioInfo?.Color || undefined,
    };
}

const ERepeatMode = NAlice.NData.ERepeatMode;
export type ITMusicPlayerData = NAlice.NData.ITMusicPlayerData;

export function AdapterGetMusicDataFromTrack({
    Track,
    ShuffleModeOn,
    RepeatMode,
    QueueItems,
    ContentInfo,
    ContentId,
}: ITMusicPlayerData, requestState: IRequestState) {
    const trackId = Track?.Id || ' ';
    const playlist: IMusicTrackInfo[] | null | undefined = compact(QueueItems?.map(el => musicTrackDataAdapter(el)));
    const currentTrack = playlist.find(el => el.id === trackId);
    const coverUri = Track?.Album?.CoverUri || playlist.find(el => el.id === trackId)?.altImageUrl || ' ';
    const artist = (Track?.Artists && Track.Artists[0] && Track.Artists[0].Name) || ' ';
    const audio_source_id = trackId;
    const header = Track?.Subtype || ' ';
    const title = Track?.Title || ' ';
    const isDisliked = requestState.hasExperiment(ExpFlags.renderPlayerLikes) ? (Track?.IsDisliked ?? undefined) : undefined;
    const isLiked = requestState.hasExperiment(ExpFlags.renderPlayerLikes) ? (Track?.IsLiked ?? undefined) : undefined;
    const isShuffle = typeof ShuffleModeOn === 'boolean' ? ShuffleModeOn : false;
    const playlistName = ContentInfo?.Title || ' ';
    const playlistId = ContentId?.Id || '';
    const imageBackground = currentTrack?.imageBackground;
    const color = imageBackground;

    return {
        coverUri,
        trackId,
        artist,
        audio_source_id,
        header,
        title,
        isDisliked,
        isLiked,
        isShuffle,
        repeatMode: RepeatMode || ERepeatMode.NONE,
        playlist,
        playlistName,
        playlistId,
        objectType: `${ContentId?.Type}`,
        imageBackground,
        color,
    };
}
