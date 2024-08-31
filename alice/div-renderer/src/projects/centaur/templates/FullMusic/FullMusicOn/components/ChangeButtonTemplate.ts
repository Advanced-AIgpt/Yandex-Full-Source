import { ContainerBlock, FixedSize, ImageBlock, SolidBackground, Template, TextBlock, WrapContentSize } from 'divcard2';
import { colorDarkGrey } from '../../../../style/constants';
import { title32m } from '../../../../style/Text/Text';

export function ChangeButtonTemplate() {
    return new ContainerBlock({
        width: new FixedSize({ value: 248 }),
        height: new FixedSize({ value: 184 }),
        border: {
            corner_radius: 28,
        },
        background: [
            new SolidBackground({
                color: colorDarkGrey,
            }),
        ],
        alignment_horizontal: 'center',
        alignment_vertical: 'center',
        items: [
            new ImageBlock({
                image_url: new Template('change_image'),
                width: new FixedSize({ value: 56 }),
                height: new FixedSize({ value: 56 }),
                margins: {
                    top: 32,
                    bottom: 20,
                },
                alignment_vertical: 'center',
                alignment_horizontal: 'center',
            }),
            new TextBlock({
                ...title32m,
                text: new Template('change_text'),
                max_lines: 1,
                alignment_horizontal: 'center',
                width: new WrapContentSize(),
                height: new WrapContentSize(),
                margins: {
                    right: 24,
                    left: 24,
                },
            }),
        ],
    });
}
