import { NAlice } from '../../../../../../protos';
import { ISkillCardButton } from '../../types';
import { ISkillCardItemsList, ISkillCardItemsListItem } from './types';
type ITButton = NAlice.NData.TDialogovoSkillCardData.TSkillResponse.ITButton;
type ITImageItem = NAlice.NData.TDialogovoSkillCardData.TSkillResponse.ITImageItem;
type ITSkillResponse = NAlice.NData.TDialogovoSkillCardData.ITSkillResponse;

function getItemListItemData(item: ITImageItem): ISkillCardItemsListItem {
    return {
        title: item.Title ?? '',
        imageUrl: item.ImageUrl ?? '',
        description: item.Description ?? '',
        button: getButtonData(item.Button),
    };
}

function getButtonData(button: Readonly<ITButton | null | undefined>): ISkillCardButton | null {
    if (button) {
        return {
            text: button.Text ?? '',
            url: button.Url ?? '',
            payload: button.Payload ?? null,
        };
    }
    return null;
}

export function getItemListData(card: Readonly<ITSkillResponse>): ISkillCardItemsList | null {
    if (card.ItemsListResponse) {
        return {
            type: 'ItemsList',
            header: {
                text: card.ItemsListResponse.ItemsLisetHeader?.Text || ' ',
            },
            items: card.ItemsListResponse.ImageItems?.map(getItemListItemData) || [],
            footer: {
                text: card.ItemsListResponse.ItemsLisetFooter?.Text || null,
                button: getButtonData(card.ItemsListResponse.ItemsLisetFooter?.Button),
            },
        };
    }

    return null;
}
