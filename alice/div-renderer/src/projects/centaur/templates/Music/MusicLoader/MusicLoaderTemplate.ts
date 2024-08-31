import { Div, Template } from 'divcard2';
import MusicDiv from '../MusicDiv';
import getColorSet from '../../../style/colorSet';
import { createRequestState } from '../../../../../registries/common';
import { IRequestState } from '../../../../../common/types/common';

export interface IMusicLoaderProps {
    header: string;
    coverURI: string;
    title: string;
    playlist_name: string;
}

export function MusicLoaderTemplate(): [Div, IRequestState] {
    const requestState = createRequestState();

    return [
        MusicDiv({
            header: new Template('header'),
            isShuffle: null,
            coverURI: new Template('coverURI'),
            colorSet: getColorSet(),
            title: new Template('title'),
            repeatMode: null,
            playlist: [],
            playlistName: new Template('playlist_name'),
            trackId: '',
            playlistId: '',
            objectType: 'Album',
            requestState: createRequestState(),
        }),
        requestState,
    ];
}
