import { NAlice } from '../../../../../protos';
import { ITeaserItemsList, ITeaserItemsListItem } from './types';
import { isEmpty } from 'lodash';


function getItemListItemData(item: NAlice.NData.TTeaserSettingsWithContentData.ITeaserSettingWithPreview): ITeaserItemsListItem {
    return <ITeaserItemsListItem>{
        title: isEmpty(item.TeaserName) ? (item.TeaserConfigData?.TeaserType ?? 'У меня почему-то нет названия') : item.TeaserName,
        teaserType: item.TeaserConfigData?.TeaserType ?? '',
        teaserId: item.TeaserConfigData?.TeaserId ?? '',
        isChosen: item.IsChosen,
    };
}

export function getItemListData(card: NAlice.NData.ITTeaserSettingsWithContentData): ITeaserItemsList {
    return {
        type: 'ItemsList',
        header: {
            text: '',
        },
        items: card.TeaserSettingsWithPreview?.map(getItemListItemData) || [],
    };
}
