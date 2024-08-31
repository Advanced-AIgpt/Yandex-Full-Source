import {
    SolidBackground,
    ContainerBlock,
    MatchParentSize,
    WrapContentSize,
    TextBlock,
    DivStateBlock,
} from 'divcard2';
import { compact } from 'lodash';
import {
    telegramMuteMicDirective,
    telegramUnmuteMicDirective,
    turnOnVideoDirective,
    turnOffVideoDirective,
} from '../../actions/client/telegramClientActions';
import { directivesAction } from '../../../../common/actions';
import { colorMoreThenBlack } from '../../style/constants';
import { title44r } from '../../style/Text/Text';
import { setStateActionInAllPlaces } from '../../../../common/actions/div';

export const callControlTriggers = [
    {
        condition: '@{isTelegramMicMuted}',
        actions: setStateActionInAllPlaces(
            {
                places: [{ name: 'top_level', place: ['0'] }],
                state: [
                    'mic_control_id',
                    'mic_control_muted_id',
                ],
                logPrefix: 'trigger_mic_unmute_on_',
            },
        ),
    },
    {
        condition: '@{!isTelegramMicMuted}',
        actions: setStateActionInAllPlaces(
            {
                places: [{ name: 'top_level', place: ['0'] }],
                state: [
                    'mic_control_id',
                    'mic_control_unmuted_id',
                ],
                logPrefix: 'trigger_mic_mute_on_',
            },
        ),
    },
    {
        condition: '@{isTelegramVideoEnabled}',
        actions: setStateActionInAllPlaces(
            {
                places: [{ name: 'top_level', place: ['0'] }],
                state: [
                    'video_control_id',
                    'video_control_enabled_id',
                ],
                logPrefix: 'trigger_video_enabled_on_',
            },
        ),
    },
    {
        condition: '@{!isTelegramVideoEnabled}',
        actions: setStateActionInAllPlaces(
            {
                places: [{ name: 'top_level', place: ['0'] }],
                state: [
                    'video_control_id',
                    'video_control_disabled_id',
                ],
                logPrefix: 'trigger_video_disabled_on_',
            },
        ),
    },
];

export const micControlButton = (
    userId: string,
    callId: string | undefined = undefined,
) => {
    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        paddings: {
            left: 40,
            top: 29,
            right: 40,
            bottom: 29,
        },
        margins: {
            left: 40,
        },
        background: [
            new SolidBackground({ color: '#ffffff' }),
        ],
        border: {
            corner_radius: 28,
        },
        items: [
            new DivStateBlock({
                div_id: 'mic_control_id',
                default_state_id: 'mic_control_unmuted_id',
                states: [
                    {
                        state_id: 'mic_control_unmuted_id',
                        div: new TextBlock({
                            ...title44r,
                            text_color: colorMoreThenBlack,
                            text: 'Mute mic',
                            actions: compact([
                                {
                                    url: directivesAction(telegramMuteMicDirective(userId, callId)),
                                    log_id: 'video_call_mic_mute_action',
                                },
                            ]),
                        }),
                    },
                    {
                        state_id: 'mic_control_muted_id',
                        div: new TextBlock({
                            ...title44r,
                            text_color: colorMoreThenBlack,
                            text: 'Unmute mic',
                            actions: compact([
                                {
                                    url: directivesAction(telegramUnmuteMicDirective(userId, callId)),
                                    log_id: 'video_call_mic_unmute_action',
                                },
                            ]),
                        }),
                    },
                ],
                width: new MatchParentSize(),
                height: new MatchParentSize(),
            }),
        ],
    });
};

export const videoControlButton = (
    userId: string,
    callId: string | undefined = undefined,
) => {
    return new ContainerBlock({
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        paddings: {
            left: 40,
            top: 29,
            right: 40,
            bottom: 29,
        },
        margins: {
            left: 40,
        },
        background: [
            new SolidBackground({ color: '#ffffff' }),
        ],
        border: {
            corner_radius: 28,
        },
        items: [
            new DivStateBlock({
                div_id: 'video_control_id',
                default_state_id: 'video_control_enabled_id',
                states: [
                    {
                        state_id: 'video_control_enabled_id',
                        div: new TextBlock({
                            ...title44r,
                            text_color: colorMoreThenBlack,
                            text: 'Video off',
                            actions: compact([
                                {
                                    url: directivesAction(turnOffVideoDirective(userId, callId)),
                                    log_id: 'video_call_video_turn_off_action',
                                },
                            ]),
                        }),
                    },
                    {
                        state_id: 'video_control_disabled_id',
                        div: new TextBlock({
                            ...title44r,
                            text_color: colorMoreThenBlack,
                            text: 'Video on',
                            actions: compact([
                                {
                                    url: directivesAction(turnOnVideoDirective(userId, callId)),
                                    log_id: 'video_call_video_turn_on_action',
                                },
                            ]),
                        }),
                    },
                ],
                width: new MatchParentSize(),
                height: new MatchParentSize(),
            }),
        ],
    });
};
