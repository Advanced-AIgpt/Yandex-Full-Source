import {
    SolidBackground,
} from 'divcard2';
import { getStaticS3Asset } from '../../../helpers/assets';
import { setStateActionInAllPlaces } from '../../../../../common/actions/div';
import {
    MUSIC_ICON_IMAGE_SIZE,
    MUSIC_ICON_SIZE,
    MUSIC_PLAYER_SECONDARY_BLOCK, MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
    MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
} from '../constants';
import { IconButton } from '../../../../../common/components/IconButton';
import { IColorSet } from '../../../style/colorSet';
import { musicPlaces } from '../../FullMusic/components/PlayerControls/components/PlayPausePlayerButton';

export const MUSIC_MORE_BUTTON = 'music_more_button';
export const MUSIC_MORE_BUTTON_ON = 'on';
export const MUSIC_MORE_BUTTON_OFF = 'off';

export function MoreButton(colorSet: Readonly<IColorSet>) {
    return IconButton({
        iconUrl: getStaticS3Asset('music/standard/playlist.png'),
        size: MUSIC_ICON_SIZE,
        iconSize: MUSIC_ICON_IMAGE_SIZE,
        id: MUSIC_MORE_BUTTON,
        color: colorSet.textColorOpacity50,
        states: [
            {
                stateId: MUSIC_MORE_BUTTON_OFF,
                background: [
                    new SolidBackground({ color: colorSet.iconsBgColor }),
                ],
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            MUSIC_MORE_BUTTON,
                            MUSIC_MORE_BUTTON_ON,
                        ],
                        logPrefix: 'change_more_music_button',
                    }),
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            MUSIC_PLAYER_SECONDARY_BLOCK,
                            MUSIC_PLAYER_SECONDARY_BLOCK_LIST,
                        ],
                        logPrefix: 'change_music_player_secondary_block',
                    }),
                ],
            },
            {
                stateId: MUSIC_MORE_BUTTON_ON,
                background: [
                    new SolidBackground({ color: colorSet.textColor }),
                ],
                color: colorSet.mainColor,
                actions: [
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            MUSIC_MORE_BUTTON,
                            MUSIC_MORE_BUTTON_OFF,
                        ],
                        logPrefix: 'change_more_music_button',
                    }),
                    ...setStateActionInAllPlaces({
                        places: musicPlaces,
                        state: [
                            MUSIC_PLAYER_SECONDARY_BLOCK,
                            MUSIC_PLAYER_SECONDARY_BLOCK_COVER,
                        ],
                        logPrefix: 'change_music_player_secondary_block',
                    }),
                ],
            },
        ],
    });
}
