import { ContainerBlock } from 'divcard2';
import { MainMusicInfoProps } from '../../MainMusicInfo';
import { PlayerDislike } from '../../PlayerDislike';
import { PlayerControls } from '../../../../components/PlayerControls/PlayerControls';
import { PlayerLike } from '../../PlayerLike';
import { IRequestState } from '../../../../../../../../common/types/common';

export function Controls({
    trackId,
    isLiked,
    isDisliked,
}: Pick<MainMusicInfoProps, 'trackId' | 'isLiked' | 'isDisliked'>, requestState: IRequestState) {
    return new ContainerBlock({
        orientation: 'horizontal',
        margins: {
            left: 14,
        },
        items: [
            PlayerDislike({
                trackId,
                isDisliked,
            }),
            PlayerControls(requestState),
            PlayerLike({
                trackId,
                isLiked,
            }),
        ],
    });
}
