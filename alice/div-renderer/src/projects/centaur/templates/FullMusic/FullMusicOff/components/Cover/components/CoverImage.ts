import {
    Div,
    DivAppearanceSetTransition,
    DivFadeTransition,
    DivScaleTransition,
    DivSlideTransition,
    FixedSize,
    ImageBlock,
} from 'divcard2';
import { MainMusicInfoProps } from '../../MainMusicInfo';
import { getImageCoverByLink } from '../../../../GetImageCoverByLink';

export function CoverImage({
    trackId,
    coverUri,
    direction,
    imageUri,
}: Pick<MainMusicInfoProps, 'trackId' | 'coverUri' | 'direction' | 'imageUri'>): Div {
    return new ImageBlock({
        id: `album_cover_${trackId}`,
        image_url: coverUri ? getImageCoverByLink(coverUri, '700x700') : (imageUri ?? ''),
        preload_required: 1,
        width: new FixedSize({ value: 480 }),
        height: new FixedSize({ value: 480 }),
        border: {
            corner_radius: 16,
        },
        alignment_vertical: 'center',
        transition_in: new DivAppearanceSetTransition({
            items: [
                new DivFadeTransition({
                    duration: 400,
                    interpolator: 'ease_out',
                }),
                new DivSlideTransition({
                    edge: direction,
                    distance: {
                        value: 24,
                    },
                    duration: 400,
                    interpolator: 'ease_out',
                }),
            ],
        }),
        transition_out: new DivAppearanceSetTransition({
            items: [
                new DivScaleTransition({
                    duration: 400,
                    interpolator: 'ease_in',
                }),
                new DivFadeTransition({
                    duration: 200,
                    interpolator: 'ease_in',
                }),
            ],
        }),
        extensions: [
            {
                id: 'centaur-audio-cover',
            },
        ],
    });
}
