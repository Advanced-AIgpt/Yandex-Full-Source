import {
    DivAppearanceSetTransition,
    DivFadeTransition,
    DivScaleTransition,
    DivSlideTransition,
    FixedSize,
    ImageBlock,
} from 'divcard2';
import { getImageCoverByLink } from '../../GetImageCoverByLink';

interface Props {
    trackId: string;
    coverUri: string;
}

export function AlbumCover({
    trackId,
    coverUri,
}: Props) {
    return new ImageBlock({
        id: `album_cover_${trackId}`,
        image_url: coverUri ? getImageCoverByLink(coverUri, '200x200') : '',
        preload_required: 1,
        width: new FixedSize({ value: 136 }),
        height: new FixedSize({ value: 136 }),
        border: {
            corner_radius: 28,
        },
        alignment_vertical: 'center',
        transition_in: new DivAppearanceSetTransition({
            items: [
                new DivFadeTransition({
                    duration: 400,
                    interpolator: 'ease_out',
                }),
                new DivSlideTransition({
                    edge: 'bottom',
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
