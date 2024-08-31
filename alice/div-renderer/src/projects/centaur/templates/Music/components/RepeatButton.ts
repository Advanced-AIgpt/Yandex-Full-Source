import { IColorSet } from '../../../style/colorSet';
import { NAlice } from '../../../../../protos';
import { AudioPlayerDivConstants } from '../../FullMusic/constants';
import { setStateActionInAllPlaces } from '../../../../../common/actions/div';
import { musicPlaces } from '../../FullMusic/components/PlayerControls/components/PlayPausePlayerButton';
import { directivesAction } from '../../../../../common/actions';
import { centaurActionRepeat, EnumRepeatMode } from '../../../../../common/actions/server/musicActions';
import { getStaticS3Asset } from '../../../helpers/assets';
import { IconButton } from '../../../../../common/components/IconButton';
import { MUSIC_ICON_IMAGE_SIZE } from '../constants';

const ERepeatMode = NAlice.NData.ERepeatMode;

interface IRepeatModeStateMap {
    readonly [ERepeatMode.NONE]: string;
    readonly [ERepeatMode.ALL]: string;
    readonly [ERepeatMode.TRACK]: string;
}

const repeatModeStateMap: IRepeatModeStateMap = {
    [ERepeatMode.NONE]: AudioPlayerDivConstants.REPEAT_SWITCHER_NONE_ID,
    [ERepeatMode.ALL]: AudioPlayerDivConstants.REPEAT_SWITCHER_ALL_ID,
    [ERepeatMode.TRACK]: AudioPlayerDivConstants.REPEAT_SWITCHER_ONE_ID,
};

export function RepeatButton(colorSet: Readonly<IColorSet>, repeatMode: NAlice.NData.ERepeatMode) {
    // При смене трэка состояние теряет свою актуальность
    const repeatButtonId = `${AudioPlayerDivConstants.REPEAT_SWITCHER_ID}_${Date.now()}`;

    const stateNone = {
        stateId: AudioPlayerDivConstants.REPEAT_SWITCHER_NONE_ID,
        alpha: 0.5,
        actions: [
            ...setStateActionInAllPlaces({
                places: musicPlaces,
                state: [
                    repeatButtonId,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ALL_ID,
                ],
                logPrefix: 'audio_player_repeat_state_all',
            }),
            {
                log_id: 'audio_player_repeat_all',
                url: directivesAction(centaurActionRepeat(EnumRepeatMode.All)),
            },
        ],
    };
    const stateAll = {
        stateId: AudioPlayerDivConstants.REPEAT_SWITCHER_ALL_ID,
        iconUrl: getStaticS3Asset('music/standard/repeat_more.png'),
        actions: [
            ...setStateActionInAllPlaces({
                places: musicPlaces,
                state: [
                    repeatButtonId,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_ONE_ID,
                ],
                logPrefix: 'audio_player_repeat_state_one',
            }),
            {
                log_id: 'audio_player_repeat_one',
                url: directivesAction(centaurActionRepeat(EnumRepeatMode.One)),
            },
        ],
    };
    const stateOne = {
        stateId: AudioPlayerDivConstants.REPEAT_SWITCHER_ONE_ID,
        iconUrl: getStaticS3Asset('music/standard/repeat_more+1.png'),
        actions: [
            ...setStateActionInAllPlaces({
                places: musicPlaces,
                state: [
                    repeatButtonId,
                    AudioPlayerDivConstants.REPEAT_SWITCHER_NONE_ID,
                ],
                logPrefix: 'audio_player_repeat_state_none',
            }),
            {
                log_id: 'audio_player_repeat_one',
                url: directivesAction(centaurActionRepeat(EnumRepeatMode.None)),
            },
        ],
    };

    return IconButton({
        size: MUSIC_ICON_IMAGE_SIZE,
        iconSize: MUSIC_ICON_IMAGE_SIZE,
        id: repeatButtonId,
        color: colorSet.textColor,
        iconUrl: getStaticS3Asset('music/standard/repeat.png'),
        states: [
            stateNone,
            stateAll,
            stateOne,
        ],
        iconType: 'square',
        defaultStateId: repeatModeStateMap[repeatMode],
    });
}
