import { SolidBackground } from 'divcard2';
import { AudioPlayerDivConstants } from '../../../constants';
import { ActionMusicPlayPayse } from '../../../actions';
import { FormActionAnalyticsPayload } from '../../../../../../../common/analytics/payload';
import { IconButton } from '../../../../../../../common/components/IconButton';
import { MUSIC_ICON_IMAGE_SIZE, MUSIC_ICON_SIZE } from '../../../../Music/constants';
import { DualScreenStates } from '../../../../../components/DualScreen/DualScreen';
import { IColorSet } from '../../../../../style/colorSet';
import { getStaticS3Asset } from '../../../../../helpers/assets';
import { IStatePlace, setStateActionInAllPlaces } from '../../../../../../../common/actions/div';
import { IRequestState } from '../../../../../../../common/types/common';
import { ElementType, Element } from '../../../../../../../common/analytics/context';

export const musicPlaces: IStatePlace[] = [
    {
        name: 'off_screen',
        place: [
            '0',
            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
            AudioPlayerDivConstants.MUSIC_SCREEN_OFF_ID,
        ],
    },
    {
        name: 'idle_screen',
        place: [
            '0',
            AudioPlayerDivConstants.MUSIC_SCREEN_ID,
            AudioPlayerDivConstants.MUSIC_SCREEN_IDLE_ID,
        ],
    },
    {
        name: 'new_music_horizontal',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateHorizontal,
        ],
    },
    {
        name: 'new_music_vertical',
        place: [
            '0',
            DualScreenStates.stateId,
            DualScreenStates.stateVertical,
        ],
    },
];

const triggers = [
    {
        condition: '@{isAudioPlaying}',
        actions: setStateActionInAllPlaces({
            places: musicPlaces,
            state: [
                AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID,
            ],
            logPrefix: 'trigger_action_play_on_',
        }),
    },
    {
        condition: '@{!isAudioPlaying}',
        actions: setStateActionInAllPlaces({
            places: musicPlaces,
            state: [
                AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID,
            ],
            logPrefix: 'trigger_action_pause_on_',
        }),
    },
];

export function PlayPausePlayerButton({ colorSet, requestState }: {
    colorSet?: IColorSet,
    requestState: IRequestState,
}) {
    requestState.variableTriggers.add(triggers);
    const actionAnalyticsPayload = FormActionAnalyticsPayload(requestState.analyticsContext
        .setElement(new Element(
            ElementType.Button,
            'PlayPauseButton',
            'Кнопка Play/Pause на музыкальном Плейере',
        )));

    return IconButton({
        id: AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
        animationSpaceSize: 1.2,
        size: MUSIC_ICON_SIZE,
        iconSize: MUSIC_ICON_IMAGE_SIZE,
        iconUrl: getStaticS3Asset('music/standard/pause.png'),
        background: colorSet && [new SolidBackground({ color: colorSet.textColor })],
        color: colorSet && colorSet.mainColor,
        states: [
            {
                stateId: AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID,
                animationOut: {
                    name: 'scale',
                    start_value: 1.0,
                    end_value: 1.2,
                    duration: 200,
                    interpolator: 'ease_in',
                },
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            `${AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID}_optimistic`,
                        ],
                        logPrefix: 'optimistic_play_on_',
                        payload: actionAnalyticsPayload,
                    }),
                    {
                        log_id: 'audio_player_toggle_play_pause',
                        url: ActionMusicPlayPayse(),
                        payload: actionAnalyticsPayload,
                    },
                ],
            },
            {
                stateId: `${AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID}_optimistic`,
                animationIn: {
                    name: 'scale',
                    start_value: 1.2,
                    end_value: 1.0,
                    duration: 200,
                    start_delay: 200,
                    interpolator: 'ease_out',
                },
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            `${AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID}_optimistic`,
                        ],
                        logPrefix: 'optimistic_play_on_',
                        payload: actionAnalyticsPayload,
                    }),
                    {
                        log_id: 'audio_player_toggle_play_pause',
                        url: ActionMusicPlayPayse(),
                        payload: actionAnalyticsPayload,
                    },
                ],
                alpha: 0.3,
            },
            {
                stateId: AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID,
                iconUrl: getStaticS3Asset('music/standard/play.png'),
                animationOut: {
                    name: 'scale',
                    start_value: 1.0,
                    end_value: 1.2,
                    duration: 200,
                    interpolator: 'ease_in',
                },
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            `${AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID}_optimistic`,
                        ],
                        logPrefix: 'optimistic_pause_on_',
                        payload: actionAnalyticsPayload,
                    }),
                    {
                        log_id: 'audio_player_toggle_play_pause',
                        url: ActionMusicPlayPayse(),
                        payload: actionAnalyticsPayload,
                    },
                ],
            },
            {
                stateId: `${AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PLAY_ID}_optimistic`,
                iconUrl: getStaticS3Asset('music/standard/play.png'),
                animationIn: {
                    name: 'scale',
                    start_value: 1.2,
                    end_value: 1.0,
                    duration: 200,
                    start_delay: 200,
                    interpolator: 'ease_out',
                },
                alpha: 0.3,
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_ID,
                            `${AudioPlayerDivConstants.PLAY_PAUSE_SWITCHER_PAUSE_ID}_optimistic`,
                        ],
                        logPrefix: 'optimistic_pause_on_',
                        payload: actionAnalyticsPayload,
                    }),
                    {
                        log_id: 'audio_player_toggle_play_pause',
                        url: ActionMusicPlayPayse(),
                        payload: actionAnalyticsPayload,
                    },
                ],
            },
        ],
    });
}
