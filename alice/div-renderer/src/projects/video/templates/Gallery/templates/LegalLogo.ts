import { FixedSize, ImageBlock, Template, WrapContentSize } from 'divcard2';

export function LegalLogo() {
    return new ImageBlock({
        alignment_horizontal: 'right',
        content_alignment_horizontal: 'right',
        alignment_vertical: 'top',
        content_alignment_vertical: 'top',
        width: new WrapContentSize(),
        height: new FixedSize({
            value: 60,
        }),
        scale: 'fit',
        margins: {
            right: 30,
            top: 52,
        },
        image_url: new Template('image_url'),
    });
}
