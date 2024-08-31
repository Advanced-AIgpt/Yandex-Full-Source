import { ContainerBlock, FixedSize, MatchParentSize } from 'divcard2';
import { offsetFromEdgeOfScreen } from '../../../../style/constants';
import { Cover } from './Cover/Cover';
import { Content } from './Content/Content';
import { IRequestState } from '../../../../../../common/types/common';

export interface MainMusicInfoProps {
    title: string;
    direction: 'bottom' | 'top' | 'left' | 'right';
    artist: string;
    trackId: string;
    coverUri?: string;
    imageUri?: string;
    isLoader?: boolean;
    isLiked: boolean;
    isDisliked: boolean;
}

export function MainMusicInfo({
    title,
    direction,
    artist,
    trackId,
    coverUri,
    imageUri,
    isLoader = false,
    isLiked,
    isDisliked,
}: MainMusicInfoProps, requestState: IRequestState) {
    return new ContainerBlock({
        orientation: 'horizontal',
        width: new MatchParentSize(),
        height: new FixedSize({ value: 504 }),
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: [
            Cover({
                trackId,
                coverUri,
                imageUri,
                direction,
            }),
            Content({
                trackId,
                title,
                artist,
                isLoader,
                isLiked,
                isDisliked,
            }, requestState),
        ],
    });
}
