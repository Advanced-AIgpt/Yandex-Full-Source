import { SolidBackground } from 'divcard2';
import { ActionMusicNext } from '../../../actions';
import { IconButton } from '../../../../../../../common/components/IconButton';
import { MUSIC_ICON_IMAGE_SIZE, MUSIC_ICON_SIZE } from '../../../../Music/constants';
import { IColorSet } from '../../../../../style/colorSet';
import { getStaticS3Asset } from '../../../../../helpers/assets';

export function NextPlayerButton({
    colorSet,
}: {
    colorSet?: IColorSet;
} = {}) {
    return IconButton({
        animationSpaceSize: 1.2,
        iconUrl: getStaticS3Asset('music/standard/next.png'),
        id: 'audio_player_next',
        size: MUSIC_ICON_SIZE,
        iconSize: MUSIC_ICON_IMAGE_SIZE,
        background: colorSet && [
            new SolidBackground({ color: colorSet.iconsBgColor }),
        ],
        actions: [
            {
                log_id: 'audio_player_next',
                url: ActionMusicNext(),
            },
        ],
        actionAnimation: {
            name: 'set',
            items: [
                {
                    name: 'scale',
                    start_value: 1.0,
                    end_value: 1.2,
                    duration: 200,
                    interpolator: 'ease_in_out',
                },
            ],
        },
    });
}
