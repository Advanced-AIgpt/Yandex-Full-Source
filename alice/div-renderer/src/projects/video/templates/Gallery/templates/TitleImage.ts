import { DivFadeTransition, ImageBlock, MatchParentSize, Template } from 'divcard2';

export function TitleImage() {
    return new ImageBlock({
        alignment_horizontal: 'left',
        content_alignment_horizontal: 'left',
        alignment_vertical: 'bottom',
        content_alignment_vertical: 'bottom',
        height: new MatchParentSize(),
        scale: 'fit',
        margins: {
            left: 20,
            right: 20,
            top: 23,
        },
        extensions: [{
            id: 'transition',
            params: {
                name: 'logo',
                target: 'logo',
            },
        }],
        transition_in: new DivFadeTransition({
            duration: 300,
            start_delay: 150,
            alpha: 0.0,
            interpolator: 'ease_out',
        }),
        transition_out: new DivFadeTransition({
            duration: 150,
            alpha: 0.0,
            interpolator: 'ease_in',
        }),
        image_url: new Template('image_url'),
    });
}
