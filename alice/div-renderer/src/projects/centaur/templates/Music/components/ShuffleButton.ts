import { IColorSet } from '../../../style/colorSet';
import { IconButton } from '../../../../../common/components/IconButton';
import { getStaticS3Asset } from '../../../helpers/assets';
import {
    MUSIC_ICON_IMAGE_SIZE,
    MUSIC_PLAYER_SHUFFLE_TRIGGER,
    MUSIC_PLAYER_SHUFFLE_TRIGGER_OFF,
    MUSIC_PLAYER_SHUFFLE_TRIGGER_ON,
} from '../constants';
import { setStateActionInAllPlaces } from '../../../../../common/actions/div';
import { directivesAction } from '../../../../../common/actions';
import { centaurShuffleOff, centaurShuffleOn } from '../../../../../common/actions/server/musicActions';
import { musicPlaces } from '../../FullMusic/components/PlayerControls/components/PlayPausePlayerButton';

export function ShuffleButton(colorSet: Readonly<IColorSet>, isShuffle: boolean) {
    return IconButton({
        iconUrl: getStaticS3Asset('music/standard/shuffle.png'),
        color: colorSet.textColor,
        id: MUSIC_PLAYER_SHUFFLE_TRIGGER,
        size: MUSIC_ICON_IMAGE_SIZE,
        iconSize: MUSIC_ICON_IMAGE_SIZE,
        iconType: 'square',
        defaultStateId: isShuffle ? MUSIC_PLAYER_SHUFFLE_TRIGGER_ON : MUSIC_PLAYER_SHUFFLE_TRIGGER_OFF,
        states: [
            {
                stateId: MUSIC_PLAYER_SHUFFLE_TRIGGER_ON,
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            MUSIC_PLAYER_SHUFFLE_TRIGGER,
                            MUSIC_PLAYER_SHUFFLE_TRIGGER_OFF,
                        ],
                        logPrefix: 'trigger_shuffle_off',
                    }),
                    {
                        log_id: 'audio_player_shuffle_off',
                        url: directivesAction(centaurShuffleOff()),
                    },
                ],
            },
            {
                stateId: MUSIC_PLAYER_SHUFFLE_TRIGGER_OFF,
                alpha: 0.5,
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            MUSIC_PLAYER_SHUFFLE_TRIGGER,
                            MUSIC_PLAYER_SHUFFLE_TRIGGER_ON,
                        ],
                        logPrefix: 'trigger_shuffle_on',
                    }),
                    {
                        log_id: 'audio_player_shuffle_on',
                        url: directivesAction(centaurShuffleOn()),
                    },
                ],
            },
        ],
    });
}
