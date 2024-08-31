import {
    FixedSize,
    GalleryBlock,
    ImageBlock, TextBlock,
} from 'divcard2';
import { compact } from 'lodash';
import { titleLineHeight } from '../../../../components/TitleLine/TitleLine';
import { Types } from '../../types';
import { title44r } from '../../../../style/Text/Text';
import { colorWhiteOpacity90, offsetFromEdgeOfScreen } from '../../../../style/constants';
import { ButtonList } from '../../../../components/ButtonList/ButtonList';

interface Props {
    data: Types;
    suggestsBlockHeight: number;
    clientHeight: number;
}

export default function BigImageSkill({ data, suggestsBlockHeight, clientHeight }: Props) {
    if (data.response.card.type !== 'BigImage') {
        throw new TypeError(`Invalid input data type "${data.response.card.type}", "BigImage" is expected`);
    }

    return new GalleryBlock({
        paddings: {
            top: titleLineHeight,
            bottom: suggestsBlockHeight,
        },
        height: new FixedSize({ value: clientHeight }),
        orientation: 'vertical',
        items: compact([
            new ImageBlock({
                width: new FixedSize({ value: 554 }),
                border: {
                    corner_radius: 28,
                },
                margins: {
                    bottom: 32,
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                image_url: data.response.card.image.imageUrl,
            }),
            new TextBlock({
                ...title44r,
                text_color: colorWhiteOpacity90,
                margins: {
                    left: offsetFromEdgeOfScreen,
                    right: offsetFromEdgeOfScreen,
                },
                text: compact([data.response.card.image.title, data.response.card.image.description]).join('\n\n'),
            }),
            data.response.buttons && ButtonList({ buttons: data.response.buttons }),
        ]),
    });
}
