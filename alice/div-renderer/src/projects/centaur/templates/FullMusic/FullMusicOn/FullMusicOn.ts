import {
    ContainerBlock,
    DivAppearanceSetTransition,
    DivFadeTransition,
    DivSlideTransition,
    IDivStateBlockState,
    MatchParentSize, TextBlock,
    WrapContentSize,
} from 'divcard2';
import { AudioPlayerDivConstants } from '../constants';
import { simpleBackground } from '../../../style/constants';
import { AudioPlayerShuffle } from './components/AudioPlayerShuffle';
import { AudioPlayerRepeat } from './components/AudioPlayerRepeat';
import { Cover } from './components/Cover';
import { Title } from './components/Title';
import { Artist } from './components/Artist';
import { NAlice } from '../../../../../protos';
import { text40m } from '../../../style/Text/Text';
import { setStateAction } from '../../../../../common/actions/div';
type ERepeatMode = NAlice.NData.ERepeatMode;

interface Props {
    trackId: string;
    coverUri: string;
    title: string;
    artist: string;
    audio_source_id: string;
    isShuffle: boolean;
    repeatMode: ERepeatMode;
}

export default function FullMusicOn({
    trackId,
    coverUri,
    title,
    artist,
    isShuffle,
    repeatMode,
}: Props): IDivStateBlockState {
    return {
        state_id: AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID,
        div: new ContainerBlock({
            background: simpleBackground,
            orientation: 'overlap',
            width: new MatchParentSize(),
            height: new MatchParentSize(),
            alignment_vertical: 'center',
            alignment_horizontal: 'center',
            action: {
                log_id: 'audio_player_next_screen',
                url: setStateAction(`0/${AudioPlayerDivConstants.MUSIC_SCREEN_ID}/${AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID}`),
            },
            action_animation: {
                name: 'no_animation',
            },
            items: [
                new ContainerBlock({
                    orientation: 'vertical',
                    width: new WrapContentSize(),
                    height: new WrapContentSize(),
                    alignment_vertical: 'center',
                    alignment_horizontal: 'center',
                    transition_in: new DivAppearanceSetTransition({
                        items: [
                            new DivFadeTransition({
                                duration: 400,
                                start_delay: 400,
                                interpolator: 'ease_out',
                            }),
                            new DivSlideTransition({
                                edge: 'bottom',
                                distance: {
                                    value: 48,
                                },
                                duration: 400,
                                start_delay: 400,
                                interpolator: 'ease_out',
                            }),
                        ],
                    }),
                    items: [
                        Cover(trackId, coverUri),
                        Title(trackId, title),
                        Artist(trackId, artist),
                        new ContainerBlock({
                            orientation: 'horizontal',
                            width: new WrapContentSize(),
                            height: new WrapContentSize(),
                            alignment_horizontal: 'center',
                            alignment_vertical: 'center',
                            margins: {
                                top: 40,
                            },
                            items: [
                                AudioPlayerShuffle(isShuffle),
                                AudioPlayerRepeat(repeatMode),
                            ],
                        }),
                        new TextBlock({
                            ...text40m,
                            text: 'R',
                            text_alignment_horizontal: 'center',
                            alpha: 0.1,
                        }),
                    ],
                }),
            ],
        }),
    };
}
