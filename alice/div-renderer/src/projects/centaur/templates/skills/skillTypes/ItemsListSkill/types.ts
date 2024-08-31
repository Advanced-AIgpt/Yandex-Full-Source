import type { ISkillCardButton } from '../../types';

export interface ISkillCardItemsListItem {
    imageUrl: string,
    title: string,
    description: string,
    button: ISkillCardButton | null,
}

export interface ISkillCardItemsList {
    type: 'ItemsList',
    header: {
        text: string | null,
    },
    items: ISkillCardItemsListItem[],
    footer: {
        text: string | null,
        button: ISkillCardButton | null,
    },
}
