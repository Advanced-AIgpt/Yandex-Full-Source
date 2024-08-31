import { SolidBackground } from 'divcard2';
import { ActionMusicPrev } from '../../../actions';
import { IconButton } from '../../../../../../../common/components/IconButton';
import { MUSIC_ICON_IMAGE_SIZE, MUSIC_ICON_SIZE } from '../../../../Music/constants';
import { IColorSet } from '../../../../../style/colorSet';
import { getStaticS3Asset } from '../../../../../helpers/assets';

export function PreviousPlayerButton({
    colorSet,
}: {
    colorSet?: IColorSet;
} = {}) {
    return IconButton({
        iconUrl: getStaticS3Asset('music/standard/prev.png'),
        animationSpaceSize: 1.2,
        id: 'audio_player_prev',
        size: MUSIC_ICON_SIZE,
        iconSize: MUSIC_ICON_IMAGE_SIZE,
        color: colorSet?.textColor,
        background: colorSet && [
            new SolidBackground({ color: colorSet.iconsBgColor }),
        ],
        actions: [
            {
                log_id: 'audio_player_prev',
                url: ActionMusicPrev(),
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
