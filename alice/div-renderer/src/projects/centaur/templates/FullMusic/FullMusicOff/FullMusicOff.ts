import { ContainerBlock, Div, IDivStateBlockState, MatchParentSize, SolidBackground } from 'divcard2';
import { colorMoreThenBlack } from '../../../style/constants';
import { Header } from './components/Header';
import { MainMusicInfo } from './components/MainMusicInfo';
import { ProgressLine } from './components/ProgressLine';
import { AudioPlayerDivConstants } from '../constants';
import { IRequestState } from '../../../../../common/types/common';

interface Props {
    direction: 'bottom' | 'top' | 'left' | 'right';
    header: string;
    title: string;
    artist: string;
    audio_source_id: string;
    trackId: string;
    coverUri?: string;
    imageUri?: string;
    isLoader?: boolean;
    isLiked: boolean;
    isDisliked: boolean;
    requestState: IRequestState;
}

export function FullMusicOffDiv({
    header,
    audio_source_id,
    direction,
    artist,
    title,
    trackId,
    coverUri,
    imageUri,
    isLoader = false,
    isLiked,
    isDisliked,
    requestState,
}: Props): Div {
    return new ContainerBlock({
        background: [
            new SolidBackground({ color: colorMoreThenBlack }),
        ],
        orientation: 'vertical',
        width: new MatchParentSize(),
        height: new MatchParentSize(),
        items: [
            Header({
                header,
                audio_source_id,
            }),
            MainMusicInfo({
                artist,
                title,
                direction,
                trackId,
                coverUri,
                imageUri,
                isLoader,
                isLiked,
                isDisliked,
            }, requestState),
            ProgressLine(),
        ],
    });
}

export default function FullMusicOff(props: Props): IDivStateBlockState {
    return {
        state_id: AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
        div: FullMusicOffDiv(props),
    };
}
