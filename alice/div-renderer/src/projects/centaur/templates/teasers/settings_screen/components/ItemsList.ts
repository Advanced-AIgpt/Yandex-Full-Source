import { Div } from 'divcard2';
import { ItemsListSeparator } from './ItemsListSeparator';
import { ItemsListItem } from './ItemsListItem';
import { ITeaserItemsListItem } from '../types';

export function ItemsList(items: ITeaserItemsListItem[]): Div[] {
    const length = items.length;
    const result: Div[] = [];

    for (let i = 0; i < length; i++) {
        if (i !== 0) {
            result.push(ItemsListSeparator());
        }

        result.push(ItemsListItem(items[i]));
    }

    return result;
}
