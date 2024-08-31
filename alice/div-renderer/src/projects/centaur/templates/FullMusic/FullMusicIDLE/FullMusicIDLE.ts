import { ContainerBlock, IDivStateBlockState, MatchParentSize, WrapContentSize } from 'divcard2';
import CentaurLava from './components/CentaurLava';
import { PlayerLine } from './components/PlayerLine';
import { AudioPlayerDivConstants } from '../constants';
import { setStateAction } from '../../../../../common/actions/div';

interface Props {
    trackId: string;
    coverUri: string;
}

export default function FullMusicIDLE({
    trackId,
    coverUri,
}: Props): IDivStateBlockState {
    return {
        state_id: AudioPlayerDivConstants.MUSIC_SCREEN_IDLE_ID,
        div: new ContainerBlock({
            orientation: 'overlap',
            width: new MatchParentSize(),
            height: new MatchParentSize(),
            action: {
                log_id: 'audio_player_exit_from_idle_mode',
                url: setStateAction(`0/${AudioPlayerDivConstants.MUSIC_SCREEN_ID}/${AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID}`),
            },
            items: [
                CentaurLava(),
                new ContainerBlock({
                    orientation: 'horizontal',
                    width: new MatchParentSize(),
                    height: new WrapContentSize(),
                    margins: {
                        top: 32,
                        bottom: 32,
                        left: 32,
                        right: 32,
                    },
                    alignment_vertical: 'bottom',
                    items: [
                        PlayerLine({
                            trackId,
                            coverUri,
                        }),
                    ],
                }),
            ],
        }),
    };
}
