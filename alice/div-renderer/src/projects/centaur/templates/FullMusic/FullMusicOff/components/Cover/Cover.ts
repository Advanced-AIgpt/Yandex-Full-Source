import { ContainerBlock, Div, FixedSize } from 'divcard2';
import { MainMusicInfoProps } from '../MainMusicInfo';
import { CoverImage } from './components/CoverImage';
import { BigLikeOnCover } from './components/BigLikeOnCover';

export function Cover({
    direction,
    coverUri,
    imageUri,
    trackId,
}: Pick<MainMusicInfoProps, 'trackId' | 'coverUri' | 'direction' | 'imageUri'>): Div {
    return new ContainerBlock({
        orientation: 'overlap',
        width: new FixedSize({ value: 504 }),
        height: new FixedSize({ value: 480 }),
        alignment_vertical: 'top',
        alignment_horizontal: 'left',
        items: [
            CoverImage({
                direction,
                coverUri,
                imageUri,
                trackId,
            }),
            BigLikeOnCover(),
        ],
    });
}
