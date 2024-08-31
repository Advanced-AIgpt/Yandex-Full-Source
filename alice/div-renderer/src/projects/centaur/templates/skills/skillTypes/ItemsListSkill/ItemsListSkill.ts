import { FixedSize, GalleryBlock } from 'divcard2';
import { compact } from 'lodash';
import { titleLineHeight } from '../../../../components/TitleLine/TitleLine';
import { Types } from '../../types';
import { ButtonList } from '../../../../components/ButtonList/ButtonList';
import { ItemsList } from './components/ItemsList';
import { ItemsListTitle } from './components/ItemsListTitle';

interface Props {
    data: Types;
    suggestsBlockHeight: number;
    clientHeight: number;
}

export default function ItemsListSkill({ data, suggestsBlockHeight, clientHeight }: Props) {
    if (data.response.card.type !== 'ItemsList') {
        throw new TypeError(`Invalid input data type "${data.response.card.type}", "ItemsList" is expected`);
    }

    return new GalleryBlock({
        paddings: {
            top: titleLineHeight,
            bottom: suggestsBlockHeight,
        },
        height: new FixedSize({ value: clientHeight }),
        orientation: 'vertical',
        items: compact([
            ItemsListTitle(data.response.card.header.text),
            ...ItemsList(data.response.card.items),
            data.response.buttons && ButtonList({ buttons: data.response.buttons }),
        ]),
    });
}
