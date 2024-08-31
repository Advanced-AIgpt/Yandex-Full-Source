import { ContainerBlock, DivStateBlock, FixedSize } from 'divcard2';
import { ChangeButton } from './ChangeButton';
import { MusicImages } from '../../images';
import { AudioPlayerDivConstants } from '../../constants';
import { NAlice } from '../../../../../../protos';
import { setStateAction } from '../../../../../../common/actions/div';
import { directivesAction } from '../../../../../../common/actions';
import { centaurActionRepeat, EnumRepeatMode } from '../../../../../../common/actions/server/musicActions';

function RepeatSwitcherNone() {
    return ChangeButton(MusicImages.repeatInactive, 'Повторять', {
        actions: [
            {
                log_id: 'audio_player_repeat_state_all',
                url: setStateAction([
                    '0',
                    AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                    AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ID,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ALL_ID,
                ]),
            },
            {
                log_id: 'audio_player_repeat_all',
                url: directivesAction(centaurActionRepeat(EnumRepeatMode.All)),
            },
        ],
    });
}

function RepeatSwitcherOne() {
    return ChangeButton(MusicImages.repeatOne, 'Повторять', {
        actions: [
            {
                log_id: 'audio_player_repeat_state_none',
                url: setStateAction([
                    '0',
                    AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                    AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ID,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_NONE_ID,
                ]),
            },
            {
                log_id: 'audio_player_repeat_none',
                url: directivesAction(centaurActionRepeat(EnumRepeatMode.None)),
            },
        ],
    });
}

function RepeatSwitcherAll() {
    return ChangeButton(MusicImages.repeatAll, 'Повторять', {
        actions: [
            {
                log_id: 'audio_player_repeat_state_one',
                url: setStateAction([
                    '0',
                    AudioPlayerDivConstants.MUSIC_SCREEN_ID,
                    AudioPlayerDivConstants.MUSIC_SCREEN_ON_ID,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ID,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ONE_ID,
                ]),
            },
            {
                log_id: 'audio_player_repeat_one',
                url: directivesAction(centaurActionRepeat(EnumRepeatMode.One)),
            },
        ],
    });
}

export function AudioPlayerRepeat(repeatMode: NAlice.NData.ERepeatMode) {
    let repeatModeId;

    switch (repeatMode) {
        case NAlice.NData.ERepeatMode.NONE:
            repeatModeId = AudioPlayerDivConstants.REPEAT_SWITCHER_NONE_ID;
            break;
        case NAlice.NData.ERepeatMode.ALL:
            repeatModeId = AudioPlayerDivConstants.REPEAT_SWITCHER_ALL_ID;
            break;
        case NAlice.NData.ERepeatMode.TRACK:
            repeatModeId = AudioPlayerDivConstants.REPEAT_SWITCHER_ONE_ID;
            break;
    }

    return new ContainerBlock({
        orientation: 'overlap',
        width: new FixedSize({ value: 275 }),
        height: new FixedSize({ value: 210 }),
        action_animation: {
            name: 'no_animation',
        },
        items: [
            new DivStateBlock({
                div_id: AudioPlayerDivConstants.REPEAT_SWITCHER_ID,
                width: new FixedSize({ value: 275 }),
                height: new FixedSize({ value: 210 }),
                alignment_horizontal: 'center',
                alignment_vertical: 'center',
                border: {
                    corner_radius: 28,
                },
                default_state_id: repeatModeId,
                states: [
                    {
                        state_id: AudioPlayerDivConstants.REPEAT_SWITCHER_NONE_ID,
                        div: RepeatSwitcherNone(),
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
                        state_id: AudioPlayerDivConstants.REPEAT_SWITCHER_ONE_ID,
                        div: RepeatSwitcherOne(),
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
                        state_id: AudioPlayerDivConstants.REPEAT_SWITCHER_ALL_ID,
                        div: RepeatSwitcherAll(),
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
