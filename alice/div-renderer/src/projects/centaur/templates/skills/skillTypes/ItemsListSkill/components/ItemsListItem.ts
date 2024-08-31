import { ContainerBlock, Div, FixedSize, ImageBlock, MatchParentSize, TextBlock, WrapContentSize } from 'divcard2';
import { colorWhiteOpacity50, offsetFromEdgeOfScreen } from '../../../../../style/constants';
import { text32r, text40m } from '../../../../../style/Text/Text';
import { ISkillCardItemsListItem } from '../types';

const imageSize = 88;
const imageCornerRadius = 24;

export function ItemsListItem(item: ISkillCardItemsListItem): Div {
    const containerProps: ConstructorParameters<typeof ContainerBlock>[0] = {
        orientation: 'horizontal',
        content_alignment_vertical: 'center',
        width: new MatchParentSize(),
        height: new WrapContentSize(),
        action: {
            log_id: 'items_list_button',
            url: item.button?.url,
        },
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        paddings: {
            top: 32,
            bottom: 34,
        },
        items: [
            new ImageBlock({
                width: new FixedSize({ value: imageSize }),
                height: new FixedSize({ value: imageSize }),
                border: {
                    corner_radius: imageCornerRadius,
                },
                margins: {
                    right: 32,
                },
                image_url: item.imageUrl,
            }),
            new ContainerBlock({
                orientation: 'vertical',
                items: [
                    new TextBlock({
                        ...text40m,
                        text: item.title,
                    }),
                    new TextBlock({
                        ...text32r,
                        text_color: colorWhiteOpacity50,
                        text: item.description,
                    }),
                ],
            }),
        ],
    };

    return new ContainerBlock(containerProps);
}
