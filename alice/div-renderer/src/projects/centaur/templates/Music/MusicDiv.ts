import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { DualScreen } from '../../components/DualScreen/DualScreen';
import { MainPlayerPart } from './MainPlayerPart';
import { SecondaryPlayerPart } from './SecondaryPlayerPart';
import { Layer } from '../../common/layers';
import { ACTION_CLOSE_MUSIC } from './constants';
import { IMusicDivProps } from './types';

export default function MusicDiv({
    header,
    title,
    colorSet,
    isShuffle,
    repeatMode,
    coverURI,
    playlist,
    playlistName,
    trackId,
    playlistId,
    objectType,
    requestState,
    isLiked,
    isDisliked,
}: IMusicDivProps) {
    return CloseButtonWrapper({
        div: DualScreen({
            requestState,
            firstDiv: [MainPlayerPart({
                requestState,
                header,
                title,
                colorSet,
                isShuffle,
                repeatMode,
                playlistName,
                renderPlaylistButton: playlist.length > 0,
            })],
            secondDiv: [SecondaryPlayerPart({
                coverURI,
                playlist,
                colorSet,
                trackId,
                playlistId,
                objectType,
                requestState,
                isLiked,
                isDisliked,
            })],
            mainColor1: colorSet.mainColor1,
            mainColor: colorSet.mainColor,
            inverseOnVertical: true,
        }),
        layer: Layer.CONTENT,
        actions: [
            {
                log_id: 'close_music_session',
                url: ACTION_CLOSE_MUSIC,
            },
        ],
    });
}
