import { DivFadeTransition, FixedSize, ImageBlock, Template } from 'divcard2';

export function Thumbnail() {
    return new ImageBlock({
        width: new FixedSize({
            value: 700,
        }),
        height: new FixedSize({
            value: 400,
        }),
        extensions: [
            {
                id: 'transition',
                params: {
                    name: 'thumbnail',
                    target: 'thumbnail',
                },
            },
        ],
        alignment_horizontal: 'right',
        alignment_vertical: 'top',
        transition_in: new DivFadeTransition({
            duration: 400,
            alpha: 0.0,
            interpolator: 'ease_out',
        }),
        image_url: new Template('image_url'),
    });
}
