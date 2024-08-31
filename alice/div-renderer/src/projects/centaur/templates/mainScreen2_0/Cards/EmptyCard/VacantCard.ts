import { Div, FixedSize, ImageBlock, MatchParentSize, TextBlock } from 'divcard2';
import { BasicCard } from '../BasicCard';
import { title32m } from '../../../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../../../style/constants';
import { getS3Asset } from '../../../../helpers/assets';
import { IEmptyCardProps } from './types';
import { getCardMainScreenId } from '../helpers';

export default function VacantCard({ actions, longtap_actions, rowIndex, colIndex } : IEmptyCardProps): Div {
    return BasicCard({
        id: getCardMainScreenId({ rowIndex, colIndex }),
        height: new MatchParentSize(),
        width: new MatchParentSize(),
        actions,
        longtap_actions,
        items: [
            new ImageBlock({
                height: new FixedSize({ value: 88 }),
                width: new FixedSize({ value: 88 }),
                margins: {
                    top: 24,
                    bottom: 24,
                },
                image_url: getS3Asset('icons/plus_in_circle.png'),
                alignment_horizontal: 'center',
            }),
            new TextBlock({
                ...title32m,
                text_color: colorWhiteOpacity50,
                width: new MatchParentSize(),
                height: new MatchParentSize(),
                text_alignment_horizontal: 'center',
                text: 'Убрать виджет',
            }),
        ],
    });
}
