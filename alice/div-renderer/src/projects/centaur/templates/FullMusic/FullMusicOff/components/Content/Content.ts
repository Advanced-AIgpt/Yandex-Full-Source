import { ContainerBlock, MatchParentSize } from 'divcard2';
import { compact } from 'lodash';
import EmptyDiv from '../../../../../components/EmptyDiv';
import { MainMusicInfoProps } from '../MainMusicInfo';
import { TitleBlock } from './components/TitleBlock';
import { ArtistBlock } from './components/ArtistBlock';
import { Controls } from './components/Controls';
import { IRequestState } from '../../../../../../../common/types/common';

export function Content({
    trackId,
    title,
    artist,
    isLoader = false,
    isLiked,
    isDisliked,
}: Pick<MainMusicInfoProps, 'trackId' | 'title' | 'artist' | 'isLoader' | 'isLiked' | 'isDisliked'>, requestState: IRequestState) {
    return new ContainerBlock({
        orientation: 'vertical',
        width: new MatchParentSize({ weight: 1 }),
        height: new MatchParentSize(),
        items: compact([
            TitleBlock({
                trackId,
                title,
            }),
            ArtistBlock({
                trackId,
                artist,
            }),
            new EmptyDiv({
                height: new MatchParentSize({ weight: 1 }),
            }),
            !isLoader && Controls({
                trackId,
                isLiked,
                isDisliked,
            }, requestState),
        ]),
    });
}
