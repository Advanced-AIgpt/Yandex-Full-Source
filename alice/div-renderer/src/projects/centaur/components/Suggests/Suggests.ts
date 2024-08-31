import { Div, GalleryBlock } from 'divcard2';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { ISuggest } from './types';
import Suggest from './Suggest';

interface ISuggestsProps {
    suggests: ISuggest[];
}

export default function Suggests({
    suggests,
}: ISuggestsProps): Div | null {
    if (suggests.length === 0) {
        return null;
    }

    return new GalleryBlock({
        id: 'suggests',
        item_spacing: 16,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
            bottom: offsetFromEdgeOfScreen,
        },
        items: suggests.map(Suggest),
    });
}
