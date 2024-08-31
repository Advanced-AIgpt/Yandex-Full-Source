import { ContainerBlock, FixedSize, ImageBlock, TextBlock } from 'divcard2';
import { compact } from 'lodash';
import CloseButton, { closeButtonDefaultSize } from '../CloseButton';
import { text40m } from '../../style/Text/Text';
import { offsetFromEdgeOfScreen, skillNameColorTitle } from '../../style/constants';

interface Props {
    title: Readonly<string>;
    needClose?: Readonly<boolean>;
    icon?: Readonly<string>;
    options?: Readonly<Partial<ConstructorParameters<typeof ContainerBlock>[0]>>;
    closeButtonOptions?: Readonly<Parameters<typeof CloseButton>[0]>;
}

const iconSize = new FixedSize({ value: 88 });

export const titleLineHeight = offsetFromEdgeOfScreen * 2 + Math.max(
    closeButtonDefaultSize,
    iconSize.value as number,
    text40m.line_height as number,
);

export default function TitleLine({ title, icon, needClose = false, options = {}, closeButtonOptions = {} }: Props) {
    const containerOptions: ConstructorParameters<typeof ContainerBlock>[0] = {
        orientation: 'horizontal',
        content_alignment_vertical: 'center',
        alignment_vertical: 'top',
        alignment_horizontal: 'left',
        paddings: {
            left: offsetFromEdgeOfScreen,
            bottom: offsetFromEdgeOfScreen,
            top: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        items: compact([
            icon && new ImageBlock({
                image_url: icon,
                width: iconSize,
                height: iconSize,
                margins: {
                    right: 32,
                },
                border: {
                    corner_radius: 44,
                },
            }),
            new TextBlock({
                ...text40m,
                text_color: skillNameColorTitle,
                text: title,
            }),
            needClose && CloseButton(closeButtonOptions),
        ]),
        ...options,
    };

    return new ContainerBlock(containerOptions);
}
