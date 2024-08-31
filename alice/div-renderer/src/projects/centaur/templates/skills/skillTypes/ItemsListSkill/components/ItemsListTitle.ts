import { Div, FixedSize, TextBlock } from 'divcard2';
import { title44m } from '../../../../../style/Text/Text';
import { offsetFromEdgeOfScreen } from '../../../../../style/constants';

export function ItemsListTitle(title?: string | null): Div | null {
    if (!title) {
        return null;
    }

    return new TextBlock({
        ...title44m,
        height: new FixedSize({ value: 88 }),
        text_alignment_vertical: 'center',
        margins: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        text: title,
    });
}
