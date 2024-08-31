import { FixedSize, ImageBlock } from 'divcard2';
import { getImageCoverByLink } from '../../GetImageCoverByLink';

export function Cover(trackId: string, coverUri: string) {
    return new ImageBlock({
        id: `album_cover_info_${trackId}`,
        image_url: getImageCoverByLink(coverUri, '200x200'),
        preload_required: 1,
        width: new FixedSize({ value: 160 }),
        height: new FixedSize({ value: 160 }),
        alignment_horizontal: 'center',
        alignment_vertical: 'center',
        border: {
            corner_radius: 20,
        },
    });
}
