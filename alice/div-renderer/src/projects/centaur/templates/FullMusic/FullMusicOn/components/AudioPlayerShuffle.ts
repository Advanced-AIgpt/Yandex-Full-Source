import { ContainerBlock, DivStateBlock, FixedSize } from 'divcard2';
import { ChangeButton } from './ChangeButton';
import { MusicImages } from '../../images';
import { AudioPlayerDivConstants } from '../../constants';
import { setStateAction } from '../../../../../../common/actions/div';
import { centaurShuffleOff, centaurShuffleOn } from '../../../../../../common/actions/server/musicActions';
import { directivesAction } from '../../../../../../common/actions';

function ShuffleSwitcherOff() {
    return ChangeButton(MusicImages.shuffleInactive, 'Перемешать', {
        actions: [
            {
                log_id: 'audio_player_shuffle_state_on',
                url: setStateAction([
                    '0',
                    AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                    AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID,
                    AudioPlayerDivConstants.SHUFFLE_SWITCHER_ID,
                    AudioPlayerDivConstants.SHUFFLE_SWITCHER_ON_ID,
                ]),
            },
            {
                log_id: 'audio_player_shuffle_on',
                url: directivesAction(centaurShuffleOn()),
            },
        ],
    });
}

function ShuffleSwitcherOn() {
    return ChangeButton(MusicImages.shuffleActive, 'Перемешать', {
        actions: [
            {
                log_id: 'audio_player_shuffle_state_off',
                url: setStateAction([
                    '0',
                    AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                    AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID,
                    AudioPlayerDivConstants.SHUFFLE_SWITCHER_ID,
                    AudioPlayerDivConstants.SHUFFLE_SWITCHER_OFF_ID,
                ]),
            },
            {
                log_id: 'audio_player_shuffle_off',
                url: directivesAction(centaurShuffleOff()),
            },
        ],
    });
}

export function AudioPlayerShuffle(isShuffle: boolean) {
    return new ContainerBlock({
        orientation: 'overlap',
        width: new FixedSize({ value: 275 }),
        height: new FixedSize({ value: 210 }),
        action_animation: {
            name: 'no_animation',
        },
        items: [
            new DivStateBlock({
                div_id: AudioPlayerDivConstants.SHUFFLE_SWITCHER_ID,
                width: new FixedSize({ value: 275 }),
                height: new FixedSize({ value: 210 }),
                default_state_id: isShuffle ?
                    AudioPlayerDivConstants.SHUFFLE_SWITCHER_ON_ID :
                    AudioPlayerDivConstants.SHUFFLE_SWITCHER_OFF_ID,
                states: [
                    {
                        state_id: AudioPlayerDivConstants.SHUFFLE_SWITCHER_OFF_ID,
                        div: ShuffleSwitcherOff(),
                        animation_in: {
                            name: 'scale',
                            start_value: 1.1,
                            end_value: 1.0,
                            duration: 300,
                            start_delay: 300,
                            interpolator: 'ease_out',
                        },
                        animation_out: {
                            name: 'scale',
                            start_value: 1.0,
                            end_value: 1.1,
                            duration: 300,
                            interpolator: 'ease_in',
                        },
                    },
                    {
                        state_id: AudioPlayerDivConstants.SHUFFLE_SWITCHER_ON_ID,
                        div: ShuffleSwitcherOn(),
                        animation_in: {
                            name: 'scale',
                            start_value: 1.1,
                            end_value: 1.0,
                            duration: 300,
                            start_delay: 300,
                            interpolator: 'ease_out',
                        },
                        animation_out: {
                            name: 'scale',
                            start_value: 1.0,
                            end_value: 1.1,
                            duration: 300,
                            interpolator: 'ease_in',
                        },
                    },
                ],
            }),
        ],
    });
}
