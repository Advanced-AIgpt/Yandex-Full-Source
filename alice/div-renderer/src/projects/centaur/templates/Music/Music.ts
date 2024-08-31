import { Div, IDivData } from 'divcard2';
import { NAlice } from '../../../../protos';
import getColorSet, { IColorSet } from '../../style/colorSet';
import { TopLevelCard } from '../../helpers/helpers';
import MusicDiv from './MusicDiv';
import { getImageCoverByLink } from '../FullMusic/GetImageCoverByLink';
import RadioDiv from './RadioDiv';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { AdapterGetMusicDataFromTrack } from './dataAdapter';
import { MediaType } from '../../../../common/analytics/context';
const EContentType = NAlice.NData.NMusic.TContentId.EContentType;

type ITMusicPlayerData = NAlice.NData.ITMusicPlayerData;

export function Music(musicPlayerData: ITMusicPlayerData, _: MMRequest, requestState: IRequestState) {
    const {
        // header,
        title,
        isShuffle,
        coverUri,
        artist,
        playlist,
        repeatMode,
        playlistName,
        trackId,
        playlistId,
        objectType,
        imageBackground,
        color,
        isLiked,
        isDisliked,
    } = AdapterGetMusicDataFromTrack(musicPlayerData, requestState);

    const colorSet: Readonly<IColorSet> = getColorSet({
        color,
    });

    const imageUrl = coverUri ? getImageCoverByLink(coverUri, '700x700') : '';

    let AudioDiv: (options: Parameters<typeof MusicDiv>[0]) => Div;

    switch (musicPlayerData.ContentId?.Type) {
        case EContentType.FmRadio:
            AudioDiv = RadioDiv;
            break;
        default:
            AudioDiv = MusicDiv;
            break;
    }

    requestState.analyticsContext = requestState.analyticsContext
        .addScreen('music_player')
        .addMediaMeta({
            mediaType: MediaType.Music,
            pictureUrl: coverUri,
            objectId: trackId,
        });

    const div: IDivData = {
        log_id: 'music',
        transition_animation_selector: 'data_change',
        states: [
            {
                state_id: 0,
                div: AudioDiv({
                    header: artist,
                    title,
                    colorSet,
                    isShuffle,
                    repeatMode,
                    coverURI: imageUrl,
                    playlist,
                    playlistName,
                    trackId,
                    playlistId,
                    objectType,
                    requestState,
                    imageBackground,
                    isLiked,
                    isDisliked,
                }),
            },
        ],
    };

    return TopLevelCard(div, requestState);
}
