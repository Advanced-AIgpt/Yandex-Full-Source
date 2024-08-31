import {
    ContainerBlock,
    FixedSize,
    GalleryBlock,
    ImageBlock, TextBlock, WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import { titleLineHeight } from '../../../../components/TitleLine/TitleLine';
import { Types } from '../../types';
import { text32r } from '../../../../style/Text/Text';
import { colorWhiteOpacity90, offsetFromEdgeOfScreen } from '../../../../style/constants';
import { ButtonList } from '../../../../components/ButtonList/ButtonList';
import { ISkillCardGalleryImage } from './types';

interface Props {
    data: Types;
    suggestsBlockHeight: number;
    clientHeight: number;
}

function GallerySkillColumn(image: ISkillCardGalleryImage, isFirst = false, isLast = false) {
    return new ContainerBlock({
        width: new WrapContentSize(),
        items: [
            new ImageBlock({
                height: new FixedSize({ value: 394 }),
                border: {
                    corner_radius: 20,
                },
                margins: {
                    bottom: 6,
                    left: isFirst ? offsetFromEdgeOfScreen : 0,
                    right: isLast ? offsetFromEdgeOfScreen : 16,
                },
                image_url: image.imageUrl || '',
            }),
            new TextBlock({
                ...text32r,
                text_color: colorWhiteOpacity90,
                margins: {
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                alpha: 0.5,
                text: compact([image.title, image.description]).join('\n'),
            }),
        ],
    });
}

export default function GallerySkill({ data, suggestsBlockHeight, clientHeight }: Props) {
    if (data.response.card.type !== 'Gallery') {
        throw new TypeError(`Invalid input data type "${data.response.card.type}", "Gallery" is expected`);
    }

    const len = data.response.card.images.length;

    return new GalleryBlock({
        paddings: {
            top: titleLineHeight,
            bottom: suggestsBlockHeight,
        },
        height: new FixedSize({ value: clientHeight }),
        orientation: 'vertical',
        items: compact([
            new GalleryBlock({
                orientation: 'horizontal',
                items: data.response.card.images
                    .map((el, index) => GallerySkillColumn(el, index === 0, index === len - 1)),
            }),
            data.response.buttons && ButtonList({ buttons: data.response.buttons }),
        ]),
    });
}
